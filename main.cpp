/**
 * @file      main.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     Examples of the MPI Error Checker usage.
 *
 * @version   Version 1.1
 *
 * @date      11 August    2020, 11:27 (created) \n
 *            09 August    2022, 22:10 (revised)
 *
 * @copyright Copyright (C) 2022 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
 *
 */

#include <cstdlib>
#include <mpi.h>
#include <vector>
#include <iostream>
#include <exception>
#include <algorithm>
#include "ErrorChecker.h"

using namespace std;

/**
 * Get rank in the communicator.
 * @param  [in] comm - MPI communicator.
 * @return rank of the process in the communicator.
 */
int mpiGetCommRank(const MPI_Comm& comm)
{
  int rank = MPI_UNDEFINED;
  MPI_Comm_rank(comm, &rank);
  return rank;
}// end of mpiGetCommRank
//----------------------------------------------------------------------------------------------------------------------

/**
 * Get size of the communicator.
 * @param  [in] comm - MPI communicator.
 * @return size of the communicator.
 */
int mpiGetCommSize(const MPI_Comm& comm)
{
  int size = MPI_UNDEFINED;
  MPI_Comm_size(comm, &size);
  return size;
}// end of mpiGetCommSize
//----------------------------------------------------------------------------------------------------------------------

/**
 * Print out the ranks that are supposed to be faulty
 * @param [in] faultyRanks
 */
void printFaultyRanks(const std::vector<int>& faultyRanks)
{
  if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
  {
    cout << "Faulty ranks throwing exception: ";
    for (auto rank : faultyRanks)
    {
      cout << rank <<", ";
    }
    cout << endl;
  }
}// end of printFaultyRanks
//----------------------------------------------------------------------------------------------------------------------

/**
 * Is this rank faulty?
 * @param [in] faultyRanks
 * @return
 */
bool isFaulty(const std::vector<int>& faultyRanks)
{
  return (std::find(faultyRanks.begin(), faultyRanks.end(), mpiGetCommRank(MPI_COMM_WORLD)) != faultyRanks.end());
}// end of isFaulty
//----------------------------------------------------------------------------------------------------------------------

/**
 * Report exception from the selected rank
 * @param [in] distException
 */
void reportError(const DistException& distException)
{
  if (distException.getDeadlockMode())
  {
    if (mpiGetCommRank(MPI_COMM_WORLD) == distException.getRank())
    {
      cout << "Exception found under deadlock situation" << endl;
      cout << "Communicator:  " << distException.getCommName()       << endl;
      cout << "Rank:          " << distException.getRank()           << endl;
      cout << "Error message: " << distException.getErrorMessage()   << endl;
      cout << "Error code:    " << int(distException.getErrorCode()) << endl;
    }
  }
  else
  {
    if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
    {
      cout << "Exception found without deadlock" << endl;
      cout << "Communicator:  " << distException.getCommName()       << endl;
      cout << "Rank:          " << distException.getRank()           << endl;
      cout << "Error message: " << distException.getErrorMessage()   << endl;
      cout << "Error code:    " << int(distException.getErrorCode()) << endl;
    }
  }
}// end of reportError
//----------------------------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------------------------//
//-------------------------------------- Example 01 throw std::runtime_error() ---------------------------------------//
//----------------------------------------------- Deadlock free mode -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Throw an std::runtime_error() in a deadlock free mode.
 * @param [in] faultyRanks - List of faulty ranks
 */
void example01(const std::vector<int>& faultyRanks)
{
  if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
  {
    cout << "------------------ Handle exception std::runtime_error ------------------" << endl;
    printFaultyRanks(faultyRanks);
  }

  try
  {
    // If I'm a faulty rank, trow exception
    if (isFaulty(faultyRanks))
    {
      throw std::runtime_error("Runtime error in rank "  + to_string(mpiGetCommRank(MPI_COMM_WORLD)));
    }

    // The rank exits the try block properly.
    ErrorChecker::setSuccess();
    if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
    {
      cout << "Successful computation" << endl;
    }
  }
   catch (const std::exception& e)
    {
      // This must be at the first line since it tells us the the error and which rank is supposed to log the error.
      const DistException& distException = ErrorChecker::catchException(e);

      reportError(distException);

      if (distException.getDeadlockMode())
      {
        MPI_Abort(MPI_COMM_WORLD, int(distException.getErrorCode()));
      }
      else
      {
        ErrorChecker::finalize();
        MPI_Finalize();
        exit(int(distException.getErrorCode()));
      }
    }
}// end of example01
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------- Example 02 throw MPI invalid rank  ----------------------------------------//
//----------------------------------------------- Deadlock free mode -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//


/**
 * Error cased by sending a message to a invalid rank
 * @param [in] faultyRanks - List of faulty ranks
 */
void example02(const std::vector<int>& faultyRanks)
{
  if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
  {
    cout << "------------------ Handle exception MPI invalid rank ------------------" << endl;
    printFaultyRanks(faultyRanks);
  }

  try
  {
    // If I'm a faulty rank, trow exception.
    if (isFaulty(faultyRanks))
    {
      int x = 0;
      // Send to invalid rank 100
      MPI_Send(&x, 1, MPI_INT, 100, 0, MPI_COMM_WORLD);
    }

    // The rank exits the try block properly.
    ErrorChecker::setSuccess();
    if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
    {
      cout << "Successful computation" << endl;
    }
  }
   catch (const std::exception& e)
    {
      // This must be at the first line since it tells us the the error and which rank is supposed to log the error.
      const DistException& distException = ErrorChecker::catchException(e);

      reportError(distException);

      if (distException.getDeadlockMode())
      {
        MPI_Abort(MPI_COMM_WORLD, int(distException.getErrorCode()));
      }
      else
      {
        ErrorChecker::finalize();
        MPI_Finalize();
        exit(int(distException.getErrorCode()));
      }
    }
}// end of example02
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//-------------------------------------- Example 03 throw std::runtime_error() ---------------------------------------//
//-------------------------------------------------- Deadlock mode ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Error cased by sending a message to a invalid rank
 * @param [in] faultyRanks - List of faulty ranks
 */
void example03(const std::vector<int>& faultyRanks)
{
  if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
  {
    cout << "------------------ Handle exception std::runtime_error under deadlock mode ------------------" << endl;
    printFaultyRanks(faultyRanks);
  }

  try
  {
    // If I'm a faulty rank, trow exception
    if (isFaulty(faultyRanks))
    {
      throw std::runtime_error("Runtime error in rank "  + to_string(mpiGetCommRank(MPI_COMM_WORLD)));
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // The rank exits the try block properly.
    ErrorChecker::setSuccess();
    if (mpiGetCommRank(MPI_COMM_WORLD) == 0)
    {
      cout << "Successful computation" << endl;
    }
  }
   catch (const std::exception& e)
    {
      // This must be at the first line since it tells us the the error and which rank is supposed to log the error.
      const DistException& distException = ErrorChecker::catchException(e);

      reportError(distException);

      if (distException.getDeadlockMode())
      {
        MPI_Abort(MPI_COMM_WORLD, int(distException.getErrorCode()));
      }
      else
      {
        ErrorChecker::finalize();
        MPI_Finalize();
        exit(int(distException.getErrorCode()));
      }
    }
}// end of example03
//----------------------------------------------------------------------------------------------------------------------


/**
 * main
 */
int main(int argc, char** argv)
{
  // Init MPI and set the error handler
  MPI_Init(&argc, &argv);

  // Initialize error checker. Since being a static class it should be done here.
  ErrorChecker::init();


  constexpr int example = 1;

  switch (example)
  {
    case 1:
    // Throw and report std::runtime_error(), ranks 1 and 2;
    {
      vector<int> ranks {1, 2};
      example01(ranks);
      break;
    }

    case 2:
    // Throw and report MPI error invalid rank, ranks 1 and 2;
    {
      vector<int> ranks {1, 2};
      example02(ranks);
      break;
    }

    case 3:
    // Throw and report std::runtime_error() while the others wait in barrier, ranks 1 and 2;
    {
      vector<int> ranks {1, 2};
      example03(ranks);
      break;
    }
  }

  MPI_Finalize();
  return 0;
}// end of main
//----------------------------------------------------------------------------------------------------------------------

