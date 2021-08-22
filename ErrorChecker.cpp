/**
 * @file      ErrorChecker.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file containing a class responsible for checking errors in distributed programs.
 *
 * @version   Version 1.0
 *
 * @date      11 August    2020, 11:27 (created) \n
 *            22 August    2021, 17:37 (revised)
 *
 * @copyright Copyright (C) 2021 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
 *
 */

#include <unistd.h>
#include <algorithm>

#include "ErrorChecker.h"



//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Initialization ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MPI::Comm&      ErrorChecker::sProtectedComm     = MPI::COMM_SELF;
MPI::Intracomm  ErrorChecker::sErrorExchangeComm = MPI::COMM_SELF;
double          ErrorChecker::sTimeout           = kDefaultTimeout;
int             ErrorChecker::sRank              = 0;
MPI::Errhandler ErrorChecker::sMpiErrorHandler   = MPI::ERRORS_ARE_FATAL;

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Initialize the error checker.
 */
void ErrorChecker::init(const MPI::Comm& protectedComm,
                        const double     timeout)
{
  // Set protected communicator.
  sProtectedComm = protectedComm;
  // Set local rank.
  sRank = sProtectedComm.Get_rank();

  // Set error handler to the protected communicator. This must be handed over a member variable to keep it alive
  // for the whole live of the application.
  sMpiErrorHandler = MPI::Comm::Create_errhandler(ErrorChecker::mpiErrrorHandler);
  sProtectedComm.Set_errhandler(ErrorChecker::sMpiErrorHandler);

  // Create a copy of the communicator.
  sErrorExchangeComm = sProtectedComm.Clone();
  sErrorExchangeComm.Set_name("ErrorExchangeComm");

  // Set timeout.
  sTimeout = timeout;
}// end of init
//----------------------------------------------------------------------------------------------------------------------

/**
 * Finalize error checker.
 */
void ErrorChecker::finalize()
{
  sErrorExchangeComm.Free();
}//end of finalize
//----------------------------------------------------------------------------------------------------------------------

/**
 * Test whether all ranks were successful.
 */
void ErrorChecker::setSuccess()
{
  // This rank in the protected communicator finished the block without any problems.
  unsigned int faultyRank = kNoRank;

  // Convert the value into unsigned int and take the minimum. We use a non-blokcing communication to prevent deadlock.
  MPI_Request req;
  MPI_Iallreduce(MPI_IN_PLACE, &faultyRank, 1, MPI_UNSIGNED, MPI_MIN, sErrorExchangeComm, &req);
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  // If some rank caused an exception, but the root didn't, the root receives the error message here.
  if ((faultyRank != kNoRank))
  {
    if (isRoot())
    { // Rethrow the exception with all data in the root rank.
      throw (receiveErrorData(faultyRank));
    }
    else
    {
      // Other ranks throw an unknown exception.
      throw ErrorChecker::Exception(DistException::ExceptionType::kUnknown,
                                    sRank,
                                    sProtectedComm,
                                    DistException::ErrorCode::kUnknown,
                                    "Unknown");
    }
  }
}// end of setSuccess
//----------------------------------------------------------------------------------------------------------------------

/**
 * Catch the exception and return error message.
 *
 * 1. Call allreduce to find out who caused the error. Only the lowest rank is returned.
 * 2. If it fails another communication is performed to find the list of faulty ranks and identify the lowest one
 * 3. If it successes the lowest error ranks sends the information to the root.
 */
const DistException ErrorChecker::catchException(const std::exception& e)
{
  // Catching the exception created other than the one rethrown by the ErrorCatcher itseft.
  if (typeid(e) != typeid(ErrorChecker::Exception))
  {
    // I'm the faulty rank.
    unsigned int faultyRank = sRank;

    // Use a non-blocking communication and wait for a predefined time before giving up.
    MPI_Request req;
    MPI_Iallreduce(MPI_IN_PLACE, &faultyRank, 1, MPI_UNSIGNED, MPI_MIN, sErrorExchangeComm, &req);

    // Busy wait and test.
    int completed = 0;
    double start = MPI_Wtime();
    while ((!completed) && ((MPI_Wtime() - start) < kDefaultTimeout))
    {
      // Busy wait
      MPI_Test(&req, &completed, MPI_STATUS_IGNORE);
    }

    // The error info exchange was not successful.
    if (!completed)
    {
      const int lowestRank = findLowestFaultyRank();

      if (typeid(e) == typeid(DistException))
      {
        // Handling distributed exception.
        DistException distException = dynamic_cast<const DistException&>(e);
        distException.setDeadlockMode(true);
        distException.setRank(lowestRank);

        return distException;
      }
      else
      { // Handling any other exception (eg., bad_alloc)
        return DistException(DistException::ExceptionType::kSystem,
                             lowestRank,
                             sProtectedComm,
                             DistException::ErrorCode::kSystem,
                             e.what(),
                             true); // deadlock mode
      }
    }
    else
    {
      // Error exchange was successful.
      if (typeid(e) == typeid(DistException))
      {  // Handling distributed exception.
        const DistException& exception = dynamic_cast<const DistException&>(e);

        // If the faulty rank is the root, it doesn't have to send the message to itself.
        // If the faulty rank is somebody else, then it sends the error message to the root.
        if (!isRoot(faultyRank))
        {
          sendErrorData(exception);
        }
        return exception;
      }
      else
      { // Handling with any other exception, e.g. bad_alloc.
        const DistException exception(DistException::ExceptionType::kSystem,
                                      sRank,
                                      sProtectedComm,
                                      DistException::ErrorCode::kSystem,
                                      e.what());
        if (!isRoot(faultyRank))
        {
          sendErrorData(exception);
        }
        return exception;
      }
    }
  }
  else
  { // Rethrown the exception caused by setSuccess in this rank.
    return dynamic_cast<const DistException&>(e);
  }
}// end of catchException
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * MPI error handler.
 */
void ErrorChecker::mpiErrrorHandler(MPI::Comm& comm, int* errorCode, ...)
{
  int  errorMessageLength = MPI::MAX_ERROR_STRING;
  char errorMessage[errorMessageLength];

  MPI::Get_error_string(*errorCode, errorMessage, errorMessageLength);

  throw DistException(DistException::ExceptionType::kMpi,
                      comm.Get_rank(),
                      comm,
                      DistException::ErrorCode::kMpi,
                      std::string(errorMessage));

}// end of mpiErrrorHandler
//----------------------------------------------------------------------------------------------------------------------

/**
 * Send error data to the communicator root.
 */
void ErrorChecker::sendErrorData(const DistException& exception)
{
  // We need to send three messages.
  MPI::Request reqs[3];

  const int exceptionType = int(exception.getType());
  const int errorCode     = int(exception.getErrorCode());

      // Receive exception type and error code.
  reqs[int(MpiTag::kExceptionType)] = sErrorExchangeComm.Isend(&exceptionType,
                                                               1,
                                                               MPI::INT,
                                                               kRootRank,
                                                               int(MpiTag::kExceptionType));

  reqs[int(MpiTag::kErrorCode)]     = sErrorExchangeComm.Isend(&errorCode,
                                                               1,
                                                               MPI::INT,
                                                               kRootRank,
                                                               int(MpiTag::kErrorCode));


  reqs[int(MpiTag::kErrorString)]   = sErrorExchangeComm.Isend(exception.getErrorMessage().c_str(),
                                                               exception.getErrorMessage().size() + 1, //final \0
                                                               MPI::CHAR,
                                                               kRootRank,
                                                               int(MpiTag::kErrorString));

  MPI::Request::Waitall(3, reqs);
}// end of sendErrorData
//----------------------------------------------------------------------------------------------------------------------

/**
 * Receive error data from the faulty rank and create an error object out of it.
 */
ErrorChecker::Exception ErrorChecker::receiveErrorData(const int faultyRank)
{
  // We need to receive three messages.
  MPI::Request reqs[3];

  // Temporary variable for enum classes being transferred as ints.
  int exceptionType;
  int errorCode;

  // Receive exception type and error code.
  reqs[int(MpiTag::kExceptionType)] = sErrorExchangeComm.Irecv(&exceptionType,
                                                               1,
                                                               MPI::INT,
                                                               faultyRank,
                                                               int(MpiTag::kExceptionType));
  reqs[int(MpiTag::kErrorCode)]     = sErrorExchangeComm.Irecv(&errorCode,
                                                               1,
                                                               MPI::INT,
                                                               faultyRank,
                                                               int(MpiTag::kErrorCode));


  // Receive error message.
  // Get size of the error message.
  MPI::Status status;
  sErrorExchangeComm.Probe(faultyRank, int(MpiTag::kErrorString), status);

  // Message length and the buffer for message
  const int messageLenght = status.Get_count(MPI::CHAR);
  char errorMessage[messageLenght];

  // Receive error message
  reqs[int(MpiTag::kErrorString)] = sErrorExchangeComm.Irecv(errorMessage,
                                                             messageLenght,
                                                             MPI::CHAR,
                                                             faultyRank,
                                                             int(MpiTag::kErrorString));
  // Wait for all messages.
  MPI::Request::Waitall(3, reqs);

  return Exception(DistException::ExceptionType(exceptionType),
                   faultyRank,
                   sProtectedComm,
                   DistException::ErrorCode(errorCode),
                   errorMessage);
}// end of receiveErrorData
//----------------------------------------------------------------------------------------------------------------------

/**
 * Find the lowest alive rank. It is implemented as a manual non-blocking error free Allgather operation.
 * All ranks that handles the exception sends their ranks to each other ranks in the communicator.
 * If some ranks are deadlocked, we consider them as not being faulty (otherwise they would be handling the exception).
 * Those ranks will never reply.
 * Next we wait for some predefined time and then check which ranks responded. Consequently we find the lowest rank,
 * and cancel message request that will never finish.
 */
int ErrorChecker::findLowestFaultyRank()
{
  const int commSize = sErrorExchangeComm.Get_size();

  // Allocate requests and rankPool.
  MPI::Request* sendReqs = new MPI::Request[commSize];
  MPI::Request* recvReqs = new MPI::Request[commSize];
  unsigned int* rankPool = new unsigned int[commSize];

  // Fill rankPool to OK message.
  std::fill(rankPool, rankPool + commSize, kNoRank);

  // Send message saying I'm a faulty rank to all other ranks including myself for the sake of simplicity.
  // Since the messages are tiny, the eager protocol will be used and a buffered send will be used.
  for (int rank = 0; rank < commSize; rank++)
  {
    sendReqs[rank] = sErrorExchangeComm.Isend(&sRank, 1, MPI::UNSIGNED, rank, int(MpiTag::kFindLowestRank));
  }

  // Receive messages from faulty ranks. The good ones will either send OK or be deadlocked and won't send anything.
  for (int rank = 0; rank < commSize; rank++)
  {
    recvReqs[rank] = sErrorExchangeComm.Irecv(&rankPool[rank], 1, MPI::UNSIGNED, rank, int(MpiTag::kFindLowestRank));
  }

  // Wait for a given time and test if all send the message.
  bool completed = 0;
  double start = MPI_Wtime();
  while ((!completed) && ((MPI_Wtime() - start) < kDefaultTimeout))
  {
    // Busy wait
    completed = MPI::Request::Testall(commSize, recvReqs);
  }

  // Even if some of the messages haven't arrived, take a look a the data and find the lowest rank.
  // (min value in the array).
  unsigned int lowestRank = *std::min_element(rankPool, rankPool + sErrorExchangeComm.Get_size());

  // Although we're heading to MPI::Abort, it is nice to cancel not finished requests.
  for (int rank = 0; rank < commSize; rank++)
  {
    // Cancel not finished send requests - since the message is tiny, all are supposed to finish.
    if (!sendReqs[rank].Test())
    {
      sendReqs[rank].Cancel();
    }

    // Cancel not finished receive requests.
    if (!recvReqs[rank].Test())
    {
      recvReqs[rank].Cancel();
    }
  }

  // Delete requests and rank pool.
  delete[] sendReqs;
  delete[] recvReqs;
  delete[] rankPool;

  // Return the lowest rank.
  return int(lowestRank);
}// end of findLowestAliveRank
//----------------------------------------------------------------------------------------------------------------------