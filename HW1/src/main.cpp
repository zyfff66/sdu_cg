#include "argparser.h"
#include "ifs.h"
#include "glCanvas.h"

// ====================================================================
// ====================================================================

int main(int argc, char* argv[]) {
	// parse the command line arguments
	ArgParser args(argc, argv);
	// create the IFS object
	IFS ifs(&args);
	// initialize GL
	glutInit(&argc, argv);
	GLCanvas::initialize(&args, &ifs);  // note this function never returns
	return 0;
}

// ====================================================================
// ====================================================================

