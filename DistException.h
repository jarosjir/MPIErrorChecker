/**
 * @file      DistException.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file containing class implementing k-Wave distributed exceptions.
 *
 * @version   Version 1.0
 *
 * @date      11 August    2020, 11:45 (created) \n
 *            22 August    2021, 17:37 (revised)
 *
 * @copyright Copyright (C) 2021 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
 *
 */


#ifndef DIST_EXCEPTIONS_H
#define DIST_EXCEPTIONS_H

#include <mpi.h>
#include <string.h>
#include <exception>
#include <map>

/**
 * @class   DistException
 * @brief   This class represent the exception thrown by the k-Wave in distributed environment.
 *
 * This exception holds several important information to decode what happened.
 * <ul>
 *   <li> Type of exception {MPI, FFTW, HDF5, Solver, Parameter, System, Unknown}. </li>
 *   <li> Rank that caused the error.</li>
 *   <li> Communicator, where it happened - for future use.</li>
 *   <li> Error code to be used for exit/abort. </li>
 *   <li> Error message to be reported to the user. </li>
 *   <li> Deadlock mode saying whether we can exit or has to call abort. </li>
 * </ul>
 */
class DistException : public std::exception
{
  public:
   /**
     * @enum  ExceptionType
     * @brief Type of the exception.
     */
    enum class ExceptionType : int
    {
      /// Error in an MPI routine.
      kMpi,
      /// System exception - any successor of std::exception.
      kSystem,
      /// Unknown exception.
      kUnknown
    };

    /**
     * @enum  ErrorCode
     * @brief User defined error codes.
     */
    enum class ErrorCode : int
    {
      /// Error in an MPI routine.
      kMpi = -1,
      /// System exception - any successor of std::exception.
      kSystem = -2,
      /// Unknown exception.
      kUnknown = -3
    };


    /**
     * @brief Full constructor.
     * @param [in] type         - Type of the exception.
     * @param [in] rank         - Faulty rank.
     * @param [in] comm         - Faulty communicator.
     * @param [in] errorCode    - Error code.
     * @param [in] errorMessage - Error message.
     * @param [in] deadlockMode - Is the code deadlocked?.
     */
    explicit DistException(const ExceptionType type,
                           const int           rank,
                           const MPI::Comm&    comm,
                           const ErrorCode     errorCode,
                           const std::string&  errorMessage,
                           const bool          deadlockMode = false);

    /**
     * @brief Simplified constructor for MPI_COMM_WORLD and current rank.
     * @param [in] type         - Type of the exception.
     * @param [in] errorCode    - Error code.
     * @param [in] errorMessage - Error message.
     */
    explicit DistException(const ExceptionType type,
                           const ErrorCode     errorCode,
                           const std::string&  errorMessage);
    /// Copy constructor.
    DistException(const DistException&) = default;
    /// Destructor.
    virtual ~DistException() = default;

    /// Assignment operator.
    DistException& operator=(const DistException&) = default;

    /// Get exception type.
    virtual ExceptionType      getType()         const noexcept { return mExecptionType; };
    /// Get string name of the exception type.
    virtual const std::string& getTypeName()     const noexcept { return sExceptionTypeNames[mExecptionType]; };

    /// Get faulty rank.
    virtual int                getRank()         const noexcept { return mRank; };
    /// Set faulty rank.
    virtual void               setRank(int rank)       noexcept { mRank = rank; };
    /// Get faulty communicator.
    virtual const MPI::Comm&   getComm()         const noexcept { return mComm; };
    /// Get faulty communicator name.
    virtual const std::string& getCommName()     const noexcept { return mCommName; };

    /// Get error code.
    virtual ErrorCode          getErrorCode()    const noexcept { return mErrorCode; };
    /// Get error message in string.
    virtual const std::string& getErrorMessage() const noexcept { return mErrorMessage; };
    /// Get error message in C style.
    virtual const char*        what()            const noexcept override  { return mErrorMessage.c_str(); };

    /// Get deadlock mode - are we in a deadlock state? By default no.
    virtual bool               getDeadlockMode() const noexcept { return mDeadlockMode; };
    /// Set deadlock mode.
    virtual void               setDeadlockMode(bool deadlockMode) noexcept { mDeadlockMode = deadlockMode; };

  private:
    /// Static map holding the exception type names.
    static  std::map<ExceptionType, std::string> sExceptionTypeNames;

    /// Type of exception.
    ExceptionType    mExecptionType;
    /// Faulty rank. In deadlock mode this is the lowest faulty rank responsible for printing the error.
    int              mRank;
    /// Faulty communicator.
    const MPI::Comm& mComm;
    /// Faulty communicator name.
    std::string      mCommName;
    /// Error code.
    ErrorCode        mErrorCode;
    /// Error message.
    std::string      mErrorMessage;
    /// Deadlock mode - In this situation, mRank is responsible for printing errors and logs.
    bool             mDeadlockMode;
};// end of DistException
//----------------------------------------------------------------------------------------------------------------------

#endif /* DIST_EXCEPTIONS_H */

