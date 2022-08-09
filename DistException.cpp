/**
 * @file      DistException.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file containing class implementing k-Wave distributed exceptions.
 *
 * @version   Version 1.1
 *
 * @date      13 August    2020, 10:31 (created) \n
 *            09 August    2022, 22:16 (revised)
 *
 * @copyright Copyright (C) 2022 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
 *
 */

#include <string>
#include "DistException.h"

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Initialization ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Initialization of the static map with matrix names in the HDF5 files.
 */
std::map<DistException::ExceptionType, std::string> DistException::sExceptionTypeNames
{
  {DistException::ExceptionType::kMpi,       "MPI error"},
  {DistException::ExceptionType::kSystem,    "System error"},
  {DistException::ExceptionType::kUnknown,   "Unknown error"},
};

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Full Constructor
 */
DistException::DistException(const ExceptionType type,
                             const int           rank,
                             const MPI_Comm&     comm,
                             const ErrorCode     errorCode,
                             const std::string&  errorMessage,
                             const bool          deadlockMode)
: exception(),
  mExecptionType(type),
  mRank(rank),
  mComm(comm),
  mCommName(""),
  mErrorCode(errorCode),
  mErrorMessage(errorMessage),
  mDeadlockMode(deadlockMode)
{
  // Initialize communicator name.
  char commName[MPI_MAX_OBJECT_NAME];
  int  commNameLength;

  MPI_Comm_get_name(mComm, commName, &commNameLength);
  mCommName = commName;
}
// end of Constructor.
//----------------------------------------------------------------------------------------------------------------------

/**
 * Simplified constructor.
 */
DistException::DistException(const ExceptionType type,
                             const ErrorCode     errorCode,
                             const std::string&  errorMessage)
  : exception(),
    mExecptionType(type),
    mRank(MPI_UNDEFINED),
    mComm(MPI_COMM_WORLD),
    mCommName(""),
    mErrorCode(errorCode),
    mErrorMessage(errorMessage),
    mDeadlockMode(false)
{
  MPI_Comm_rank(MPI_COMM_WORLD, &mRank);

  // Initialize communicator name.
  char commName[MPI_MAX_OBJECT_NAME];
  int  commNameLength;

  MPI_Comm_get_name(mComm, commName, &commNameLength);
  mCommName = commName;
}
// end of Simplified constructor.
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//