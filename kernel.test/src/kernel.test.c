/*
 https://pubs.opengroup.org/onlinepubs/7908799/xsh/signal.h.html

 https://stackoverflow.com/questions/13240893/including-source-files-from-another-c-project-in-eclipse-cdt


 * As a new user I could not comment on Itay Gal's answer, so this is a really klunky way to add to that answer. My additions may be stating the obvious, but it was not all that obvious to me.

From Itay Gal You have to define 2 things:

from the referring project

1. Go to Project->Properties->C/C++ general->Paths and Symbols. Select the Source Location tab and click the Link Folder..., Then select the
folder you want to include. To "select the folder you want to include", I used Link Folder->Advanced, clicked the "Link to Folder in the
file system" check-box and browsed to my folder's location, which is in my workspace location. The "folder name" is the sub directory of my
referring project in which my referenced files will appear and was filled in automatically when I selected my referenced folder.
You can edit the folder name, but you cannot give it a path so as to make it a sub-directory.

2. Go to Project->Properties->C/C++ general->Paths and Symbols. Select the Includes tab. Make sure you select the correct language on the
left menu (for example: GNU C++) and click Add..., Then select the folder you want to include.

Expanding upon "select the folder you want to include", from the "includes" tab, I clicked Add->Workspace, selected the referenced project
and browsed to the referenced folder and clicked OK

I did not make my external project as a library, just a few files kept in another git repository which do not even build by themselves.
I wanted them separate because the code is used in common by several other projects.

To make sure you did everything OK, go to : Project->Properties->C/C++ Build->Settings. Select the GCC C++ Linker and make sure that the All
options contains "-L/your_folder_name". Repeat this for GCC C++ Compiler also.

I found my linked project under Project->Properties->C/C++Build->Settings->Cross ARM C++ Compiler in the "all options" window,
but not in the ->Cross ARM C++ Linker tab "all options" window. My code seems to build properly and properly launch a debug window anyway.

Many thanks to Itay gal for the answer.

So it worked great for my first project, as reported above. Then I tried to do the same thing with two other projects which needed to use the
referenced code. After spending a long bit of time, I concluded that the project includes were different in the "Sources" directory than they
were in the overall project. If you select the project and apply step 2 you get a different result than if you select the "Source" directory and
apply step 2. This might be something I managed to inadvertently turn on in floundering around learning about eclipse, but if it is I see nowhere
to turn it off.
 */



/*
 Eclipse linked resources are not what you think they are.
You will need to specify the headers and objects (libraries) to your major project in your test project.
I presume you are having Eclipse CDT create the makefiles for you.
If so,

o) You will need to have each unit test in a separate project. Eclipse will not create a makefile with more than one executable,
o) The easiest place to add the links is Project --> Properties --> C/C++ General --> Paths and Symbols
o) If you are using Paths and Symbols, you will need to enable the CDT Managed Build Settings provider in Project --> Properties --> C/C++ General --> Preprocessor Include Paths, Macros, etc. --> Providers tab to get the Indexer to see them


You can also provide a symlink to the necessary headers and source in your unit test projects.

You may also want to specify the project build order in
Window --> General --> Preferences --> Workspace --> Build
This order will be used for Project --> Build All
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>


#include "../include/kernel.test.h"
#include "../include/mock.memory.h"
#include "../include/mock.cpu.h"
#include "../include/mock.console.h"

#include "../../kernel/include/kernel.h"
#include "../../shared/include/test.h"

#include "../../shared/include/kiss.structs.h"

// TODO: quitar
#include "../include/planner.h"


static void test_kernel();


int main(void) {

	t_test* test = test_open("kernel.test");
	test_kernel();
	test_close(test);

	pthread_exit(NULL);

}


static void test_kernel() {
	// set de datos para test
	t_console_producer producer = PLANIFICADOR;


	printf("kernel.test :: run mock-memory\n");
	if (mock_memory_run() != 0) {
		printf("mock-memory :: no iniciado\n");
		goto mock_finally;
	}

	printf("kernel.test :: run mock-cpu\n");
	if (mock_cpu_run() != 0) {
		printf("mock-cpu :: no iniciado\n");
		goto mock_finally;
	}
	usleep(1000000);

	printf("kernel.test :: kernel execute\n");
	kernel_execute(LOG_LEVEL_TRACE, "/home/utnso/git/tp-2022-1c-Los-o-os/kernel.test/p.kernel.config");
	usleep(2000000);

	printf("kernel.test :: run mock-console\n");
	if (mock_console_run(producer) != 0) {
		printf("mock-console :: no iniciado\n");
	}

	// tiempo en que dejo vivo al programa
	usleep(300000000);

	printf("kernel.test :: stop mock-console\n");
	mock_console_stop();
	usleep(2000000);

	printf("kernel.test :: stop kernel\n");
	kernel_terminate();
	usleep(10000000);

mock_finally:

	printf("kernel.test :: stop mock-cpu\n");
	mock_cpu_stop();
	usleep(2000000);

	printf("kernel.test :: stop mock-memory\n");
	mock_memory_stop();
	usleep(2000000);

	printf("kernel.test :: terminado\n");
}

