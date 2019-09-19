/* Matrix processing in mpi . 
 * Elements are replaced by the average of their neighbours 
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mpi.h"
#include "matrix.h"
#define MainProcess 0
#define UseRows 2

// Number of rows each process calculates
int rows;
// Number of extra rows the first row gets (if the split isn't even)
int frow = 0;

/**
 * TO DO
 * -----
 * - Implement Way to Send multiple rows to a single process
 * - See if you can determine a matrix's size/dimension without being passed it?
 * - Document!
 * 
 */

int parse_args (int argc, char *argv[], int *fd, int *np)
{
  if ((argc != 4) ||
      ((fd[0] = open (argv[1], O_RDONLY)) == -1) ||
      ((fd[1] = open (argv[2], O_WRONLY | O_CREAT, 0666)) == -1) ||
      ((*np = atoi (argv[3])) <= 0))
    {
      fprintf (stderr,
	       "Usage: mpirun -np dimension %s matrixA matrixB dimension\n", argv[0]);
      return (-1);
    }
  return 0;
}


int main (int argc, char *argv[])
{
  int me, row, col, fd[2], i, j, r, nprocs, dim;

  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &me);
  MPI_Comm_size (MPI_COMM_WORLD, &nprocs);


  if (me == MainProcess)
    {				/* parent code */

      if (parse_args (argc, argv, fd, &dim) < 0)
      {
        MPI_Finalize ();
        exit (EXIT_FAILURE);
      }
    }

  /* broadcaste dim to all */
  MPI_Bcast (&dim, 1, MPI_INT, MainProcess, MPI_COMM_WORLD);

  // if (dim != nprocs) {
  //     if (me == MainProcess)
  //       fprintf (stderr,
  //       "Usage: mpirun -np dimension %s matrixA matrixB dimension\n", argv[0]);
  //     MPI_Finalize ();
  //     exit (EXIT_FAILURE);
  // }

  if (dim == nprocs || nprocs > dim) {
    rows = 1;
  } else if (dim%nprocs == 0) {
    rows = dim/nprocs;
  } else {
    rows = dim/nprocs;
    frow = dim%nprocs;
  }

  int totalRows = UseRows + rows;

  int A[dim + 1][dim + 1], P[dim + 1][dim + 1], superA[dim + 2][dim + 2];
  int Arow[totalRows][dim + 2], Prow[rows][dim+2];
  //int *Prow = (int *)malloc(rows*(dim + 2)*sizeof(int));

  if (me == MainProcess) {				
    /* parent code */
    /* initialize A */
    for (i = 1; i < dim + 1; i++)
      if (get_row (fd[0], dim, i, &A[i][1]) == -1) {
          fprintf (stderr, "Initialization of A failed\n");
          goto fail;
      }

    /* initialize supersized matrix */
    for (row = 0; row < dim + 2; row++)
      superA[row][0] = superA[row][dim + 1] = 0;
    for (col = 0; col < dim + 2; col++)
      superA[0][col] = superA[dim + 1][col] = 0;
    for (row = 1; row < dim + 1; row++)
      for (col = 1; col < dim + 1; col++)
        superA[row][col] = A[row][col];
  }

  /* Scatter super sized A */
  for (i = 0; i < totalRows; i++) {
    if (MPI_Scatter (&superA[i],
		     (dim + 2)*rows,
		     MPI_INT,
		     &Arow[i],
		     (dim + 2)*rows,
		     MPI_INT, MainProcess, MPI_COMM_WORLD) != MPI_SUCCESS)
      {
        fprintf (stderr, "Scattering of A failed\n");
        goto fail;
      }
  }
  
  int count = -1;
  
  if (me > dim) MPI_Finalize ();

  for (r = 1; r < rows + 1; r++) {
      for (i = 1; i < dim + 1; i++) {
        Prow[r][i] = -Arow[r][i];
        for (j = r-1; j < UseRows + r; j++) {
          printf("%d %d %d\n", Arow[j][i - 1], Arow[j][i], Arow[j][i + 1] );
          if (Arow[j][i - 1] != 0) count++;
          if (Arow[j][i] != 0) count++;
          if (Arow[j][i + 1] != 0) count++;
          Prow[r][i] += Arow[j][i - 1] + Arow[j][i] + Arow[j][i + 1];
        }
        printf("%d\n",count);
        Prow[r][i] /= count;
        count = -1;
        printf("Process %d: Element: %d\n", me , Prow[r][i]);
      }
      printf("New row!\n");
      printf("r = %d\n",r);
  }
  
  /* Gather rows of P */
  if (MPI_Gather (&Prow[1][0],
      (dim+1)*rows,
      MPI_INT,
      &P[1][0],
      (dim+1)*rows,
      MPI_INT, MainProcess, MPI_COMM_WORLD) != MPI_SUCCESS)
    {
      fprintf (stderr, "Gathering of Product  failed\n");
      goto fail;
    }
    // if (MPI_Gather (&Prow[0],
    // 	  dim + 1,
    // 	  MPI_INT,
    // 	  &P[1][0],
    // 	  dim + 1,
    // 	  MPI_INT, MainProcess, MPI_COMM_WORLD) != MPI_SUCCESS)
    //   {
    //     fprintf (stderr, "Gathering of Product  failed\n");
    //     goto fail;
    //   }


  /* write the matrix to a file */
  if (me == MainProcess) {
    for (i = 1; i < dim + 1; i++)
      if (set_row (fd[1], dim, i, &P[i][1]) == -1)
      {
        fprintf (stderr, "Writing of matrix C failed\n");
        goto fail;
      }
  }

  MPI_Finalize ();
  exit (EXIT_SUCCESS);

fail:
  fprintf (stderr, "%s aborted\n", argv[0]);
  MPI_Finalize ();
  exit (EXIT_FAILURE);
}
