/**
 * @file      ErrorChecker.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file containing a class responsible for checking errors in distributed programs.
 *
 * @version   Version 1.1
 *
 * @date      11 August    2020, 11:27 (created) \n
 *            09 August    2022, 14:45 (revised)
 *
 * @copyright Copyright (C) 2022 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
 *
 */

#ifndef ERROR_CHECKER_H
#define ERROR_CHECKER_H

#include <mpi.h>
#include <string>
#include <limits>
#include <map>

#include "DistException.h"

/**
 * @class   ErrorChecker
 * @brief   Static class implementing the exception handling.
 *
 * This class is used to report and handle exceptions in distributed algorithms. \n
 * The errors in k-Wave are reported by instances of DistException, or by system exceptions thrown by other libraries
 * based on the std::exception.\n
 *
 * Every try block shall be finished by the <tt>setSuccess</tt> method that informs the others that no exception has
 * been thrown by this rank. If any other ranks yet throws an exception, this routine will inform the current rank by
 * throwing a private instance of DistException that an error happened at a remote rank. \n
 * When handling the exception, the <tt>checkException</tt> shall be called. This routine collect the error information
 * from the other ranks and picks the best rank to log the error message.
 *
 * After processing the exception, the code finalizes and exits with an error code. However, if the try block contains
 * a collective communication, e.g. as part of FFTW or HDF5, the execution will deadlock because the faulty ranks are
 * never going to engage in the collective operation since handling the exception. In such a case, ErrorChecker
 * collect error data from the faulty ranks and picks the lowest rank to overtake the responsibility of reporting
 * info and error messages.
 *
 * This class uses its own communicator to prevent interference with other communications. This communicator is derived
 * from the one set by the init routine. The ranks has to have the same mapping between the monitored comm and
 * the internal communicator used for error message exchange. At the time being, only MPI_COMM_WORLD makes sense.
 * Moreover, the class sets a new error handler for the protected communicator.
 */
class ErrorChecker
{
  public:
    /// Default constructor not allowed, static class.
    ErrorChecker() = delete;
    /// Copy constructor not allowed, static class.
    ErrorChecker(const ErrorChecker&) = delete;
    /// Destructor not allowed, static class.
    ~ErrorChecker() = delete;

    /// Operator = not allowed, static class.
    ErrorChecker& operator=(const ErrorChecker&) = delete;

    /**
     * @brief   Initialize the Error checker.
     * @details This routine first sets a custom error handler to the protected communicator, then clone the comm
     *          to isolate error message exchange from the other transfers. \n
     *          <b> This routine shall be called immediately after MPI_Init.</b>
     * @param [in] protectedComm - Communicator with ranks to be protected by the checker. This communicator is cloned
     *                             inside for message exchange. Currently it must be MPI_COMM_WORLD.
     * @param [in] timeout       - How long to wait for the others before a violent termination of possibly deadlocked
     *                             ranks. Default value is 10s.
     */
    static void init(const MPI_Comm& protectedComm = MPI_COMM_WORLD,
                     const double     timeout      = kDefaultTimeout);

    /// Finalize the error checker and release all resources.
    static void finalize();

    /**
     * @brief Inform the others that no exception has been thrown by this rank since the beginning of the try-catch
     *        block or since the last setSuccess call.
     *
     * After sending an OK message, the routine checks whether the others also completed the try-catch block. If the
     * test fails, all throw an exception. Only the root receives detailed information on the exception.
     * This routine shall be called at the end of every try-catch block or before printing logs confirming the success.
     * There must be at least one instance of this routine in the try-catch block.
     *
     * @throw ErrorChecker::Exception - If the error happens, private ErrorChecker::Exception is thrown.
     */
    static void setSuccess();

    /**
     * @brief Catch the exception and return DistException object with detailed information.
     *
     * If any rank causes an exception, this routine is called to inform the others that something wrong happened.
     * The lowest faulty rank also sends the error message to the root rank. If the communication fails because any
     * of the ranks has entered another collective communication, the error exchange fails.
     * In such a situation another communication via non-blocking point to point transfers is performed, the list of
     * faulty ranks is identified and the lowest one overtakes the responsibility to report errors.\n
     * This routine shall be called in every catch block.
     *
     * @param   [in] e - Exception catch in the catch block.
     * @return  Distributed exception. It has meaningful information only on the root rank and the faulty ranks.
     * @warning When the exception is sent from one rank to another, the original exception class is lost and a private
     *          exception is thrown.
     */
    static const DistException catchException(const std::exception& e);

  private:
   /**
    * @class   Exception
    * @brief   This exception is used after the successful rank receives exception about the error in the setSuccses
    *          method. It only holds meaningful information on the root rank.
    * @details It is not legal to throw this exception outside of this class since it is used to distinguish between
    *          local C++ exceptions and a remotely thrown ones. Once DistException is thrown, the checking for remote
    *          exceptions is disabled to prevent infinite loop and deadlock.
    */
    class  Exception: public DistException
    {
      public:
        /**
         * @brief Constructor.
         * @param [in] type         - Type of the exception.
         * @param [in] rank         - Faulty rank.
         * @param [in] comm         - Faulty communicator.
         * @param [in] errorCode    - Error code.
         * @param [in] errorMessage - Error message.
         */
        explicit Exception(const ExceptionType            type,
                           const int                      rank,
                           const MPI_Comm&                comm,
                           const DistException::ErrorCode errorCode,
                           const std::string&             errorMessage)
          : DistException(type, rank, comm, errorCode, errorMessage)
        {};
    };// end of Exception

    /**
     * @enum  MpiTag
     * @brief Tags used to send internal messages.
     */
    enum class MpiTag: int
    {
      /// MPI tag for exception type exchange.
      kExceptionType  = 0,
      /// MPI tag for error code exchange.
      kErrorCode      = 1,
      /// MPI tag for error message exchange.
      kErrorString    = 2,
      /// MPI Tag for finding the lowest faulty rank.
      kFindLowestRank = 3
    };

    /**
     * @brief This is an MPI error hander converting MPI errors into DistException.
     * @param [in]     comm      - Protected communicator.
     * @param [in,out] errorCode - Error code.
     * @param  ...               - Other parameters given by the MPI standard.
     */
    static void mpiErrrorHandler(MPI_Comm* comm, int* errorCode, ...);

    /**
     * @brief Send error data to the root of the communicators.
     * @param [in] exception - Exception whose data are to be sent.
     */
    static void sendErrorData(const DistException& exception);

    /**
     * @brief  Receive the exception data from the faulty ranks.
     * @param  [in] faultyRank - Faulty rank.
     * @return Internal exception with the data.
     */
    static ErrorChecker::Exception receiveErrorData(const int faultyRank);

    /**
     * @brief   Find the lowest rank throwing an exception in a deadlock state.
     * @details Looks for all faulty ranks that has not entered another collective communication. The one with
     *          the lowest rank is returned. This then prints the error message.
     * @return  The lowest rank at alive ranks.
     */
    static int findLowestFaultyRank();

    /**
     * @brief  Is the rank root in a protected/error communicator?
     * @param  [in] rank - Rank to check, by default we ask about current rank.
     * @return true      - If the process has rank 0 in the communicator.
     */
    static bool isRoot(int rank = sRank) { return rank == kRootRank; };

    /// Rank of the root MPI process - the one who reports errors.
    static constexpr int    kRootRank = 0;

    /// Default timeout in seconds.
    static constexpr double kDefaultTimeout = 10;

    /// Illegal rank values to specify no one caused the error.
    static constexpr unsigned int kNoRank = std::numeric_limits<unsigned int>::max();

    /// Protected communicator.
    static MPI_Comm       sProtectedComm;
    /// Communicator used to exchange errors.
    static MPI_Comm       sErrorExchangeComm;

    /// MPI error handler.
    static MPI_Errhandler sMpiErrorHandler;

    /// How long to wait before violent abort.
    static double sTimeout;
    /// Local rank in both the protected and ErrorExchange communicator.
    static int    sRank;
};// Error checker
//----------------------------------------------------------------------------------------------------------------------

#endif /* ERROR_CHECKER_H */
