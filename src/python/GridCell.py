# The width and height of the grid in units of grid cells (as opposed to pixels)
gridWidthInGridCells = 20
gridHeightInGridCells = 20

def getGridCellIdentifier(imgWidth, imgHeight, x, y):
    # Calculate the width and height of a single grid cell:
    gridCellWidth = imgWidth / gridWidthInGridCells
    gridCellHeight = imgHeight / gridHeightInGridCells

    # Calculate the position in units of grid cells:
    # (unrounded, will do it at the end)
    gridCellX = (x / gridCellWidth)
    gridCellY = (y / gridCellHeight)
    
    #xOfGridCell = gridCellX * gridCellWidth

    # Calculate the identifier (a zero-based index, so this is just converting 2D array access with x and y into 1D)
    # WRONG: cellIdentifier = round(gridCellX) + round(gridCellY * gridCellWidth)
    cellIdentifier = round(gridCellX) + round(gridCellY * gridWidthInGridCells)
    return cellIdentifier
