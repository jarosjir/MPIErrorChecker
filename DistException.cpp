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
 * @version   Version 1.0
 *
 * @date      13 August    2020, 10:31 (created) \n
 *            22 August    2021, 17:37 (revised)
 *
 * @copyright Copyright (C) 2021 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
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
                     const MPI::Comm&    comm,
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
  char commName[MPI::MAX_OBJECT_NAME];
  int  commNameLength;

  mComm.Get_name(commName, commNameLength);
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
    mRank(MPI::COMM_WORLD.Get_rank()),
    mComm(MPI::COMM_WORLD),
    mCommName(""),
    mErrorCode(errorCode),
    mErrorMessage(errorMessage),
    mDeadlockMode(false)
{
  // Initialize communicator name.
  char commName[MPI::MAX_OBJECT_NAME];
  int  commNameLength;

  mComm.Get_name(commName, commNameLength);
  mCommName = commName;
}
// end of Simplified constructor.
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//