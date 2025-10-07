#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define NUM_OF_DIRS 4

/***************************** * Data structures * ***********************************/

typedef struct GridS
{
  int numBlockedCells;
  int rows;
  int cols;
  int** cells;
} GridS;

typedef struct CoordS
{
  int row;
  int col;
}CoordS;

typedef struct PathS
{
  int cover;
  int len;
  CoordS* path;
}PathS;

CoordS dirs[NUM_OF_DIRS] = {{1,0}, {-1,0}, {0,1}, {0,-1}};

/***************************** * Local function declarations * ***********************************/

static GridS* createGrid(const int rows,
                         const int cols,
                         const CoordS* const blockedCells,
                         const int numBlockedCells);
static void printGrid(const GridS* const grid, const int rows, const int cols);
static void solve(const GridS* const grid, const int numMoves);
static void findPath(const GridS* const grid,
                     int* cacheMap,
                     const int row,
                     const int col,
                     const int numMoves,
                     const int movesLeft,
                     PathS* curPath,
                     PathS* bestPath,
                     int* visitedCells,
                     const int visitId);
static void freeGrid(GridS* grid);
static void printResult(const PathS* const bestPath);

static inline bool isCellValid(const GridS* const grid, const int row, const int col)
{
  return row >= 0 && row < grid->rows && col >= 0 && col < grid->cols && !grid->cells[row][col];
}

static inline int getCellIdx(const GridS* const grid, const int row, const int col)
{
  return row * grid->cols + col;
}

static inline int getCacheMapIdx(const GridS* const grid,
                                 const int row,
                                 const int col,
                                 const int numMoves,
                                 const int move)
{
  return (row * grid->cols + col) * numMoves + move;
}

/************************************************************************************************/

int main()
{
  const int maxMoves = 25;
  const int rows = 8, cols = 8;
  const CoordS blockedCells[] = {{2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {3,3}, {4,3}, {5,5},{6,6}};
  int numBlockedCells = sizeof(blockedCells)/sizeof(CoordS);
  GridS* grid = createGrid(rows, cols, blockedCells, numBlockedCells);
  printGrid(grid, rows, cols);
  solve(grid, maxMoves);
  freeGrid(grid);
  return 0;
}

/***************************** * Local function definitions * ***********************************/

static GridS* createGrid(const int rows,
                         const int cols,
                         const CoordS* const blockedCells,
                         const int numBlockedCells)
{
  GridS* grid = (GridS*)malloc(sizeof(GridS));
  if (!grid)
  {
    perror("Failed: malloc grid");
    exit(EXIT_FAILURE);
  }
  grid->rows = rows;
  grid->cols = cols;
  grid->numBlockedCells = numBlockedCells;

  grid->cells = malloc(rows * sizeof(int*));
  if (!grid->cells)
  {
    perror("Failed: malloc cells");
    free(grid);
    exit(EXIT_FAILURE);
  }
  
  for (int row = 0; row < rows; row++)
  {
    // set values to 0(unblocked) by default
    grid->cells[row] = calloc(cols, sizeof(int));
  }

  // Set blocked cells
  for (int i = 0; i < numBlockedCells; i++)
  {
    int row = blockedCells[i].row;
    int col = blockedCells[i].col;
    if (row >= 0 && row < rows && col >= 0 && col < cols)
    {
      grid->cells[row][col] = 1;
    }
  }
  return grid;
}

static void printGrid(const GridS* const grid, int rows, int cols)
{
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      printf("%d ", grid->cells[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

// Main solver, tries to find best coverage path starting from every unloblocked cell
static void solve(const GridS* const grid, const int numMoves)
{
  if (grid->rows * grid->cols <= grid->numBlockedCells)
  {
    printf("No free cells.\n");
    return;
  }

  // Cache for memoization, stores best coverage seen at (row,col,movesLeft)
  int* cacheMap = calloc(grid->rows * grid->cols * numMoves, sizeof(int));
  if (!cacheMap)
  {
    perror("Failed: malloc cacheMap");
    exit(EXIT_FAILURE);
  }

  // Path structures for current search and best solution
  PathS curPath = {
    .cover = 1,
    .len = 1,
    .path = malloc((numMoves) * sizeof(CoordS))
  };
  if (!curPath.path)
  {
    perror("Failed: malloc curPath.path");
    exit(EXIT_FAILURE);
  }

  PathS bestPath = {
    .cover = 1,
    .len = 1,
    .path = malloc((numMoves) * sizeof(CoordS))
  };
  if (!bestPath.path)
  {
    perror("Failed: malloc bestPath.path");
    exit(EXIT_FAILURE);
  }

  // Track visited cells with visit IDs to avoid reset in each new search
  int* visitedCells = calloc(grid->rows * grid->cols, sizeof(int));
  int visitId = 1;

  // Start searching
  for (int row = 0; row < grid->rows; row++)
  {
    for (int col = 0; col < grid->cols; col++)
    {
      if (grid->cells[row][col] != 0) continue;

      // Reset curPath and visited array, set new visit ID
      curPath.cover = 1;
      curPath.len = 1;
      curPath.path[0] = (CoordS){row, col};
      int idx = getCellIdx(grid, row, col);
      visitId++;
      visitedCells[idx] = visitId;

      // Start DFS search
      findPath(grid, cacheMap, row, col, numMoves, numMoves-1,
               &curPath, &bestPath, visitedCells, visitId);
    }
  }

  printResult(&bestPath);

  // Cleanup
  free(cacheMap);
  free(curPath.path);
  free(bestPath.path);
  free(visitedCells);
}

// Recursive DFS with memoization to explore all paths
static void findPath(const GridS* const grid,
                     int* cacheMap,
                     const int row,
                     const int col,
                     const int numMoves,
                     const int movesLeft,
                     PathS* curPath,
                     PathS* bestPath,
                     int* visitedCells,
                     const int visitId)
{
  // Memoization check: skip if no improvement possible
  int cachMapIdx = getCacheMapIdx(grid, row, col, numMoves, movesLeft);
  if (cacheMap[cachMapIdx] >= curPath->cover) return;
  cacheMap[cachMapIdx] = curPath->cover;

  // // Update path record
  curPath->path[curPath->len-1] = (CoordS){row, col};
  if (curPath->cover > bestPath->cover ||
      (curPath->cover == bestPath->cover && curPath->len < bestPath->len))
  {
    bestPath->cover = curPath->cover;
    bestPath->len = curPath->len;
    memcpy(bestPath->path, curPath->path, curPath->len * (sizeof(CoordS)));
  }

  // Stop if no moves
  if (movesLeft == 0) return;

  // Search in 4 directions
  for (int dir = 0; dir < NUM_OF_DIRS; dir++)
  {
    int nRow = row + dirs[dir].row;
    int nCol = col + dirs[dir].col;

    if (!isCellValid(grid, nRow, nCol)) continue;

    int idx = getCellIdx(grid, nRow, nCol);
    bool isUniqueCell = (visitedCells[idx] != visitId);

    // Moving
    // Increment cover if it's unique cell
    if (isUniqueCell) curPath->cover++;
    curPath->len++;
    visitedCells[idx] = visitId;

    findPath(grid, cacheMap, nRow, nCol, numMoves, movesLeft-1, curPath, bestPath, visitedCells, visitId);

    // Backtrack for the next interation
    curPath->len--;
    if (isUniqueCell)
    {
      curPath->cover--;
      visitedCells[idx] = 0;
    }
  }
}

static void freeGrid(GridS* grid)
{
  if (!grid) return;
  for (int row = 0; row < grid->rows; row++) free(grid->cells[row]);
  free(grid->cells);
  free(grid);
}

static void printResult(const PathS* const bestPath)
{
  printf("Best coverage: %d\n", bestPath->cover);
  printf("Path length: %d\n", bestPath->len);
  for (int i = 0; i < bestPath->len; i++) {
    printf("(%d,%d)%s", bestPath->path[i].row, bestPath->path[i].col,
            (i+1==bestPath->len) ? "\n" : " -> ");
  }
}