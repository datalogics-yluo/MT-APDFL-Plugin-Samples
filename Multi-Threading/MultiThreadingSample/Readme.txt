	File structure is CPlusPlus\Sample_Source\MultiThreading\MutliThreadingSample\

	The header MTHeader.h will need to be included in any specific multi-threading sample. It contains defintions for platform independent thread start/end and critical sections. 

	it also contains object defintions for a "workerthread" base object. as well as utilitiy objects to parse command lines. 

	The module MultiThreadingSample contains the skeletons fo worker objects to run PDF/a, PDF/x, XPS2PDF, and TextExtraction, as well as a NonAPDFL thread.

	The body will read a command line in the form:

	"TotalThreads=nnn ActiveThreads=nnn BaseLib (true|false) Processes=[list of threads to run in order] PDFaOption=[...] PDFxOption=[...]. etc"

    The body will then create a list of N thread infobocks, alternating though the type of processes specifed in Processes, then run that list "ActiveThreads" threads at a time.

    This will let us test no only "Multi Thread PDF/a", but also "Multi thread PDF/a and PDF/x", for any comibination. 

		Add a new class to add a new process.

		What I have to complete is the body (build the lsit, and run them N at a time), The setup for each of the 5 current worker threads (What files do we want them to use, and such like), and the worker threads themselves (which are fairly small). I put the library init/term, and the collection of statistics into the base object (workerthread) so it does not need to repeat for each type of worker. Only the setup and actual thread body need to be defined for each type of worker.

