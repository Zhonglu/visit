// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

#include <Python.h>
#include <Py2and3Support.h>

// get select
#if defined(_WIN32)
#include <winsock2.h>
#else
#include <strings.h>   // bzero by way of FD_ZERO
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// The following 2 include lines are only for the MIPSpro 7.41 compiler.
// There is some conflict between Python.h and Utility.h in including
// those 2 files.  Remove once support for MIPSpro is dropped.
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <FileFunctions.h>
#include <Utility.h>
#include <InstallationFunctions.h>
#include <StringHelpers.h>
#include <VisItException.h>
#include <DebugStream.h>

#include <string>
#include <vector>

#ifdef WIN32
  #define VISITCLI_API __declspec(dllimport)

  #define BEGINSWITHQUOTE(A) (A[0] == '\'' || A[0] == '\"')

  #define ENDSWITHQUOTE(A) (A[strlen(A)-1] == '\'' || A[strlen(A)-1] == '\"')

  #define HASSPACE(A) (strstr(A, " ") != NULL)
#else
  #define VISITCLI_API
#endif

// For the VisIt module.
extern "C" void cli_initvisit(int, bool, int, char **, int, char **);
extern "C" void cli_runscript(const char *);

// ****************************************************************************
// Function: main
//
// Purpose:
//   This is the main function for the cli program.
//
// Notes:      Most of the program is in visitmodule.C which is also built as
//             a shared library.
//
// Programmer: Brad Whitlock
// Creation:   Fri Dec 7 15:34:11 PST 2001
//
// Modifications:
//   Brad Whitlock, Tue Dec 11 10:47:55 PDT 2001
//   I fixed the -debug, -s, -nowin arguments.
//
//   Brad Whitlock, Fri May 10 10:23:47 PDT 2002
//   I added the -o argument for opening databases.
//
//   Brad Whitlock, Mon Dec 16 13:43:20 PST 2002
//   I added the -verbose argument.
//
//   Brad Whitlock, Tue Jul 15 12:59:22 PDT 2003
//   I moved the code to run a script into the visit module.
//
//   Brad Whitlock, Mon Jul 28 16:51:22 PST 2003
//   Added code to prevent -sessionfile, -format, -framestep, and -output
//   from being passed on to the viewer.
//
//   Hank Childs, Thu Apr  1 08:57:11 PST 2004
//   Prevent -fps from being passed as well.
//
//   Brad Whitlock, Wed May 4 08:34:04 PDT 2005
//   I changed the code so argv[0] gets passed to the cli in argv2. I also
//   made Python initialize its threading.
//
//   Dave Bremer, Wed Mar 14 17:58:15 PDT 2007
//   Pass commandline arguments through to the main interpreter.
//
//   Jeremy Meredith, Wed Jun  6 16:41:45 EDT 2007
//   Add the -forceinteractivecli argument (equivalent to -i in python).
//
//   Brad Whitlock, Fri Jun 8 10:57:02 PDT 2007
//   Added support for saving the arguments after -s to their own tuple.
//
//   Brad Whitlock, Fri Jun 15 16:49:53 PST 2007
//   Load the visitrc file on startup.
//
//   Hank Childs, Thu Nov  8 15:49:38 PST 2007
//   Add support for ignoring nohups.
//
//   Gunther H. Weber, Thu Feb  7 14:44:20 PST 2008
//   Check for visitrc in golobal .visit directory to enable site-wide
//   macros.
//
//   Kathleen Bonnell, Thu Jun 26 17:22:55 PDT 2008 
//   Add special handling of '-s' and '-o' args on Windows, to ensure proper
//   parsing of paths-with-spaces. 
//
//    Jeremy Meredith, Thu Aug  7 15:01:14 EDT 2008
//    Assume Python won't modify argv, and cast a string literal to
//    a const.
//
//    Mark C. Miller, Tue Apr 21 14:24:18 PDT 2009
//    Added logic to manage buffering of debug logs; an extra 'b' after level.
//
//    Mark C. Miller, Wed Jun 17 14:27:08 PDT 2009
//    Replaced CATCHALL(...) with CATCHALL.
//
//    Cyrus Harrison, Mon Jun 29 15:58:02 PDT 2009
//    If a script file is passed - add its directory to the end of sys.path
//    to enable easy importing.
//
//    Cyrus Harrison, Wed Sep 30 07:53:17 PDT 2009
//    Added book keeping to track execution stack of source files and the
//    script file passed via the '-s' command line option.
//
//    Kathleen Bonnell, Wed Mar 24 08:20:01 MST 2010
//    Fix pointer overwrite problem when both "-s" and "-o" are used on Windows
//
//    Jeremy Meredith, Fri Mar 26 13:11:46 EDT 2010
//    Allow for the -o command line option to take an optional ,<pluginID>
//    suffix, e.g. "-o foobar,LAMMPS_1.0".
//
//    Hank Childs, Thu Oct 28 11:51:05 PDT 2010
//    If an exception is uncaught, then print out the information to the screen
//    and hang.  This hang will let folks using the CLI through GUI see the
//    failure.
//
//    Hank Childs, Fri Oct 29 12:32:26 PDT 2010
//    Change while (1) to select based on guidance from Sean Ahern.
//
//    Cyrus Harrison, Thu Sep 29 15:04:16 PDT 2011
//    Add PySide Support.
//
//    Cyrus Harrison, Thu Apr 12 17:33:16 PDT 2012
//    Update to reflect changes made in visit python module revamp.
//
//    Kathleen Biagas, Fri May 4 14:05:27 PDT 2012  
//    Use GetVisItLibraryDirectory to find lib location. 
//    SetIsDevelopmentVersion when -dv encountered.
//
//    Kathleen Biagas, Thu May 24 19:20:19 MST 2012  
//    Ensure visit's lib dir has path-separators properly escaped on Windows
//    before being passed to the pjoin command.
//
//    Brad Whitlock, Wed Jun 20 11:37:23 PDT 2012
//    Added -minimized argument to minimize the cli window on Windows.
//
//    Kathleen Biagas, Thu May 24 19:20:19 MST 2012  
//    Ensure runFile has path-separators properly escaped on Windows.
//
//    Cyrus Harrison, Wed Jul 23 16:16:49 PDT 2014
//    Change the cli to act more like a standard python interpreter:
//     1) Switch to using args after "-s" as sys.argv
//     2) add "-ni"  + "-non-interactive" command line switches.
//
//    Cyrus Harrison, Wed Jan  6 11:39:57 PST 2021
//    Added py2to3 option. When enabled, auto converts any python
//    source run with "Source" to use the Python 2 to 3 translator
//    before execution.
//
//    Cyrus Harrison, Wed Feb 24 16:09:45 PST 2021
//    Adjustments for Pyside 2 support. 
//
//    Kathleen Biagas, Fri Sep 24 08:36:43 PDT 2021
//    When processing args, look for '-sla' or '-la' and skip the next arg,
//    as '-s' is a viable option that could be passed to srun via -sla or -la
//    and we don't that option to be proccessed as a cli '-s' option.
//
// ****************************************************************************

int
main(int argc, char *argv[])
{
    int  retval = 0;
    int  debugLevel = 0;
    bool bufferDebug = false;
    bool verbose = false;
    bool s_found = false;
    bool py2to3 = false;
    bool pyside = false;
    bool pyside_gui = false;
    bool pyside_viewer = false;
    char *runFile = 0;
    char *loadFile = 0;
    char **argv2 = new char *[argc];
    char **argv_after_s = new char *[argc];

    int    argc_py_style = 0;
    char **argv_py_style = new char *[argc];
    
    bool scriptOnly = false;

    int i=0;

    int argc2 = 0;
    int argc_after_s = 0; 
    char* uifile = 0;
    const char* pyuiembedded_str = "-pyuiembedded"; //pass it along to client

    bool sleep_mode = false;
    std::string run_code = "";

#ifdef WIN32
    char tmpArg[512];
#endif

#ifdef IGNORE_HUPS
    signal(SIGHUP, SIG_IGN);
#endif

    // Parse the arguments
    for(i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-debug") == 0)
        {
            debugLevel = 1;
            if (i+1 < argc && isdigit(*(argv[i+1])))
               debugLevel = atoi(argv[i+1]);
            else
               fprintf(stderr,"Warning: debug level not specified, "
                              "assuming 1\n");
            if (i+1 < argc && *(argv[i+1]+1) == 'b')
                bufferDebug = true;
            i++;
            if (debugLevel < 0)
            {
                debugLevel = 0;
                fprintf(stderr,"Warning: clamping debug level to 0\n");
            }
            if (debugLevel > 5)
            {
                debugLevel = 5;
                fprintf(stderr,"Warning: clamping debug level to 5\n");
            }
        }
#ifdef WIN32
        else if((strcmp(argv[i], "-s") == 0 && (i+1 < argc)) ||
                (strcmp(argv[i], "-o") == 0 && (i+1 < argc)) ||
                (strcmp(argv[i], "-runcode") == 0 && (i+1 < argc)))
        {
            bool runF = (strcmp(argv[i], "-s") == 0);
            bool runO = (strcmp(argv[i], "-o") == 0);
            ++i;
            // append all parts of this arg back into one string
            if (BEGINSWITHQUOTE(argv[i]) && !ENDSWITHQUOTE(argv[i]))
            {
                strcpy(tmpArg, argv[i]);
                int nArgsSkip = 1;
                size_t tmplen = strlen(argv[i]);
                for (int j = i+1; j < argc; j++)
                {
                    nArgsSkip++;
                    strcat(tmpArg, " ");
                    strcat(tmpArg, argv[j]);
                    tmplen += (strlen(argv[j]) +1);
                    if (ENDSWITHQUOTE(argv[j]))
                        break;
                }
                i += (nArgsSkip -1);
                // We want to remove the beginning and ending quotes, to 
                // ensure proper operation further on.
                strncpy(tmpArg, tmpArg+1, tmplen-2);
                tmpArg[tmplen-2] = '\0';
            }
            else 
            {
                sprintf(tmpArg, "%s", argv[i]);
            }
            if (runF)
            {
                runFile = new char [strlen(tmpArg)+1];
                sprintf(runFile, "%s", tmpArg);
                s_found = true;
            }
            else if(runO)
            {
                loadFile = new char [strlen(tmpArg)+1];
                sprintf(loadFile, "%s", tmpArg);
            }
            else
            {
                run_code = tmpArg; 
            }
        }
#else
        else if(strcmp(argv[i], "-sla") == 0 && (i+1 < argc))
        {
            // skip next arg, since it may be '-s'
            // Pass the array along to the visitmodule.
            argv2[argc2++] = argv[i];
            argv_after_s[argc_after_s++] = argv[i];
            argv2[argc2++] = argv[i+1];
            argv_after_s[argc_after_s++] = argv[i+1];

            ++i;
        }
        else if(strcmp(argv[i], "-la") == 0 && (i+1 < argc))
        {
            // skip next arg, since it may be '-s'
            argv2[argc2++] = argv[i];
            argv_after_s[argc_after_s++] = argv[i];
            argv2[argc2++] = argv[i+1];
            argv_after_s[argc_after_s++] = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "-s") == 0 && (i+1 < argc))
        {
            runFile = argv[i+1];
            ++i;

            s_found = true;
        }
        else if(strcmp(argv[i], "-o") == 0 && (i+1 < argc))
        {
            loadFile = argv[i+1];
            ++i;
        }
        else if(strcmp(argv[i], "-runcode") == 0 && (i+1 < argc))
        {
            run_code = argv[i+1];
            ++i;
        }
#endif
        else if(strcmp(argv[i], "-verbose") == 0)
        {
            verbose = true;
        }
        else if(strcmp(argv[i], "-sleepmode") == 0)
        {
            sleep_mode = true;
        }
        else if(strcmp(argv[i], "-forceinteractivecli") == 0)
        {
            // Force the python interpreter to behave interactively
            // even if there is no TTY.  This lets clients hook
            // into visit's cli using pipes, and is the equivalent
            // of the "-i" flag in python.
            Py_InteractiveFlag++;
            Py_InspectFlag++;
        }
        else if( (strcmp(argv[i], "-ni") == 0) || (strcmp(argv[i], "-non-interactive") == 0))
        {
            scriptOnly = true;
            Py_InteractiveFlag=0;
            Py_InspectFlag=0;
            
        }
        // These are all movie commands and should go into the
        // argv_after_s array rather that being skipped over. Also
        // there additional movie args so the CLI should just ignore
        // them.

        // else if(strcmp(argv[i], "-output") == 0)
        // {
        //     // Skip the argument that comes along with -output. Note that
        //     // the -output argument will be accessible in sys.argv but it
        //     // is not passed along in the args that are sent to the viewer.
        //     ++i;
        // }
        // else if(strcmp(argv[i], "-format") == 0)
        // {
        //     // Skip the argument that comes along with -format.
        //     ++i;
        // }
        // else if(strcmp(argv[i], "-framestep") == 0)
        // {
        //     // Skip the argument that comes along with -framestep.
        //     ++i;
        // }
        // else if(strcmp(argv[i], "-sessionfile") == 0)
        // {
        //     // Skip the argument that comes along with -sessionfile.
        //     ++i;
        // }
        // else if(strcmp(argv[i], "-fps") == 0)
        // {
        //     // Skip the rate that comes along with -fps.
        //     ++i;
        // }
        else if(strcmp(argv[i], "-py2to3") == 0)
        {
            py2to3 = true;
        }
        else if(strcmp(argv[i], "-pyside") == 0)
        {
            pyside = true;
        }
        else if(strcmp(argv[i], "-pysideviewer") == 0)
        {
            pyside_gui = true;
            pyside_viewer = true;
            argv2[argc2++] = const_cast<char*>(pyuiembedded_str); //pass it along to client
        }
        else if(strcmp(argv[i], "-pysideclient") == 0)
        {
            pyside_gui = true;
            argv2[argc2++] = const_cast<char*>(pyuiembedded_str); //pass it along to client
        }
        else if(strcmp(argv[i], "-uifile") == 0)
        {
            pyside_gui = true;
            uifile = argv[i+1];
            ++i;
            argv2[argc2++] = const_cast<char*>(pyuiembedded_str); //pass it along to client
        }
        else if(strcmp(argv[i], "-dv") == 0)
        {
            SetIsDevelopmentVersion(true);
        }

        else if(strcmp(argv[i], "-minimized") == 0)
        {
#ifdef WIN32
            HWND console = GetConsoleWindow();
            if(console != NULL)
                ShowWindow(console, SW_MINIMIZE);
#endif
        }
        else if(strcmp(argv[i], "-hide_window") == 0)
        {
#ifdef WIN32
            HWND console = GetConsoleWindow();
            if(console != NULL)
                ShowWindow(console, SW_HIDE);
#endif
        }
        else
        {
            // Pass the array along to the visitmodule.
            argv2[argc2++] = argv[i];

            // This argument is after -s file.py so count it unless it's
            // the -dv argument.
            if(s_found && 
               strcmp(argv[i], "-dv") != 0)
            {
                argv_after_s[argc_after_s++] = argv[i];
            }
        }
    }

    //for some reason if the viewer requests the pysideviewer
    //for now this does not make sense
    for(int i = 0; i < argc; ++i)
    {
        std::string tmp = argv[i];
        if(pyside_gui && tmp == "-reverse_launch")
        {
            std::cerr << "Warning: Cannot enable pysideviewer from remote process,"
                    << " disabling and reverting functionality to pyside"
                    << std::endl;
            pyside_gui = false;
            pyside = true;
        }
        if(pyside_gui && tmp == "-nowin")
        {
            std::cerr << "Error: Cannot create pyside client and nowin at the same time" 
                      << std::endl;
            return (0);
        }

        if(pyside_gui && tmp == "-defer")
        {
            std::cerr << "Warning: Cannot use pyside client and defer at the same time"
                      << std::endl;
            return (0);
        }
        if(run_code.length() > 0 && tmp == "-reverse_launch") {
           run_code = ""; //don't execute scripts during reverse launches..
        }
        if(sleep_mode == true && tmp == "-reverse_launch") {
           sleep_mode = false; //don't execute scripts during reverse launches..
        }
    }

    TRY
    {

        // 
        // If there is a file named "visit.py" in the current working directory,
        // this will be imported instead of the actual visit python module.
        // This is the expected  python interpreter behavior, but it is very 
        // confusing when users stumble upon this. 
        //
        // We decided to provide an error message to let users identify this case.
        //
        
        FileFunctions::VisItStat_t vstat_info;
        if(FileFunctions::VisItStat("visit.py", &vstat_info) != -1)
        {
            std::ostringstream oss;
            oss <<"!!!! - WARNING - !!!!" 
                << std::endl
                <<"You have a file named 'visit.py' in your current working "
                <<"directory. Python's standard module import logic will use "
                <<"this 'visit.py' file instead of the VisIt python module "
                <<"that implements VisIt's python client interface."
                << std::endl
                <<"This will most likely disable all VisIt python client "
                <<"features!"
                << std::endl
                <<"To avoid this please run in a directory without a file "
                <<"named 'visit.py'."
                << std::endl;

            std::cout << oss.str() << std::endl;
            debug1    << oss.str() << std::endl;
        }
        
        // Initialize python
        Py_Initialize();
        PyEval_InitThreads();
        Py_SetProgramName(argv[0]);

        // prep sys.argv
        // to confirm to python conventions, we want sys.argv to include runFile and argv_after_s

        if(runFile != 0)
        {
            argv_py_style[argc_py_style] = runFile;
            argc_py_style++;
        }

        for(i=0;i<argc_after_s;i++)
        {
            argv_py_style[argc_py_style] = argv_after_s[i];
            argc_py_style++;
        }

        if(argc_py_style == 0)
        {
            PySys_SetArgv(1, const_cast<char**>(&argv[0]));
        }
        else
        {
            PySys_SetArgv(argc_py_style, argv_py_style);
        }
                
        PyRun_SimpleString((char*)"import sys");
        PyRun_SimpleString((char*)"import os");
        PyRun_SimpleString((char*)"from os.path import join as pjoin");

        // add lib to sys.path to pickup various dylibs.
        std::string vlibdir  = GetVisItLibraryDirectory(); 
        std::ostringstream oss;

        oss << "sys.path.append(pjoin(r'" << vlibdir  <<"','site-packages'))";
        PyRun_SimpleString(oss.str().c_str());

        // Initialize the VisIt module.
        cli_initvisit(bufferDebug ? -debugLevel : debugLevel,
                      verbose,
                      argc2, argv2,
                      argc_after_s, argv_after_s);

        // import visit after the module is fully inited
        PyRun_SimpleString((char*)"import visit");
        PyRun_SimpleString((char*)"import visit_utils");
        PyRun_SimpleString((char*)"from visit_utils.builtin import *");

        // enable auto 2to3 support for passed scripts
        if(py2to3)
        {
            // let folks know this is on:
            std::cout << "VisIt CLI: Automatic Python 2to3 Conversion Enabled"
                      << std::endl;
            PyRun_SimpleString("visit_utils.builtin.SetAutoPy2to3(True)");
        }

        // add original args to visit.argv_full, just in case
        // some one needs to access them.
        
        PyObject *visit_module = PyImport_AddModule("visit"); //borrowed
        PyObject *visit_dict   = PyModule_GetDict(visit_module); //borrowed
        PyObject *py_argv_full = PyList_New(argc);
    
        for (int i = 0; i < argc; i++) 
        {
            // takes over reference
            PyList_SET_ITEM(py_argv_full, i, PyString_FromString(argv[i]));
        }

        PyDict_SetItemString(visit_dict, "argv_full", py_argv_full);

        if(pyside || pyside_gui)
        {
            int error = 0;
            if(!error) error |= PyRun_SimpleString((char*)"import PySide2");
            if(!error) error |= PyRun_SimpleString((char*)"from PySide2.QtCore import *");
            if(!error) error |= PyRun_SimpleString((char*)"from PySide2.QtGui import *");
            if(!error) error |= PyRun_SimpleString((char*)"from PySide2.QtOpenGL import *");
            if(!error) error |= PyRun_SimpleString((char*)"from PySide2.QtUiTools import *");

            if(!error) error |= PyRun_SimpleString((char*)"import visit_utils.builtin.pyside_support");
            if(!error) error |= PyRun_SimpleString((char*)"import visit_utils.builtin.pyside_hook");

            if(error)
            {
                std::cerr   << "Error: Unable to initialize PySide components" 
                            << std::endl;
                return (0); 
            }
            else
            {
                // Cyrus Note, Wed Feb 24 10:15:52 PST 2021
                // This Event handler seems to make the CLI unusable.
                PyRun_SimpleString((char*)"visit_utils.builtin.pyside_support.SetupTimer()");
                PyRun_SimpleString((char*)"visit_utils.builtin.pyside_hook.SetHook()");
            }
        }

        if(pyside_gui)
        {
            //pysideviewer needs to be executed before visit import
            //so that visit will use the window..
            // we will only have one instance, init it
            int error = PyRun_SimpleString((char*)"import visit_utils.builtin.pyside_gui");

            if(error)
            {
                std::cerr   << "Error: Unable to initialize PySide GUI components" 
                            << std::endl;
                return (0);
            }

            PyRun_SimpleString((char*)"args = sys.argv");
            if(uifile) //if external file then start VisIt in embedded mode
                PyRun_SimpleString((char*)"args.append('-pyuiembedded')"); //default to embedded
            PyRun_SimpleString((char*)"tmp = visit_utils.builtin.pyside_gui.PySideGUI.instance(args)");
            PyRun_SimpleString((char*)"visit.InitializeViewerProxy(tmp.GetViewerProxyPtr())");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetRenderWindow");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetRenderWindowIds");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetUIWindow");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetPlotWindow");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetOperatorWindow");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetOtherWindow");
            PyRun_SimpleString((char*)"from visit_utils.builtin.pyside_support import GetOtherWindowNames");

            if(!uifile && !pyside_viewer)
                PyRun_SimpleString((char*)"GetUIWindow().show()");
        }

        // setup source file and source stack variables
        PyRun_SimpleString((char*)"import os\n"
                                  "__visit_script_file__  = '<None>'\n"
                                  "__visit_source_file__  = None\n"
                                  "__visit_source_stack__ = [] \n");

        PyRun_SimpleString((char*)"visit.Launch()");
        
        // reload symbols from visit, since they may have changed
        PyRun_SimpleString((char*)"from visit import *");

        // If a visitrc file exists, execute it.
        std::string visitSystemRc(GetSystemVisItRCFile());
        std::string visitUserRc(GetUserVisItRCFile());

        FileFunctions::VisItStat_t s;
        std::string visitrc;
        if(FileFunctions::VisItStat(visitUserRc.c_str(), &s) == 0)
        {
            visitrc = visitUserRc;
        }
        else if (FileFunctions::VisItStat(visitSystemRc.c_str(), &s) == 0)
        {
            visitrc = visitSystemRc;
        }

        if(!uifile && (pyside_viewer || pyside_gui))
            PyRun_SimpleString((char*)"visit.ShowAllWindows()");

        if(uifile && pyside_gui)
        {
            std::ostringstream pycmd;
            pycmd << "visit.Source(\"" << uifile << "\")";
            PyRun_SimpleString(pycmd.str().c_str());
        }

        if (visitrc.size())
        {
            PyRun_SimpleString("ClearMacros()");
            cli_runscript(visitrc.c_str());
        }

        // If a database was specified, load it.
        if(loadFile != 0)
        {
            std::ostringstream command;
            std::vector<std::string> split = SplitValues(loadFile, ',');
            
            if(split.size() == 2)
            {
                command << "OpenDatabase(\"" << split[0] << "\", 0, \"" << split[1] << "\")";
            }  
            else
            {
                command << "OpenDatabase(\"" << loadFile << "\")";
            }
            PyRun_SimpleString(command.str().c_str());
            
#ifdef WIN32
             delete [] loadFile;
#endif
        }

        // If there was a file to execute, do it.
        if(runFile !=0)
        {
            // add the script file's dir to the cli's sys.path
            std::string pycmd  = "import sys\n";
            pycmd += "__visit_script_file__ = os.path.abspath(r'";
            pycmd +=  std::string(runFile) + "')\n";
            pycmd += "__visit_script_path__ = ";
            pycmd += "os.path.split(__visit_script_file__)[0]\n";
            pycmd += "sys.path.append(__visit_script_path__)\n";

            PyRun_SimpleString(pycmd.c_str());
            
            cli_runscript(runFile);

#ifdef WIN32
             delete [] runFile;
#endif
        }
        ///run the commandline script..
        if(run_code.length() > 0) {
            PyRun_SimpleString(run_code.c_str());
        }

        if(sleep_mode == true) {
            PyRun_SimpleString("import time\nwhile 1: time.sleep(5)");
        }
        else if(!scriptOnly)
        {
            PyObject *rl_module = PyImport_ImportModule("readline");
            if (rl_module == NULL)
                PyErr_Clear();
            else
                Py_DECREF(rl_module);
            retval = PyRun_InteractiveLoop(stdin,"<stdin>");
        }

        Py_Finalize();

    }

    CATCH(VisItException &e)
    {
        cerr << "A fatal exception occurred when trying to launch the CLI." << endl;
        cerr << "The following information may help a VisIt developer debug the problem: " << endl;
        cerr << "Message: " << e.Message() << endl;
        cerr << "Type: " << e.GetExceptionType() << endl;
        cerr << "Location: " << e.GetFilename() << ", " << e.GetLine() << endl;
        cerr << "(You must press Ctrl-C to get the program to stop)" << endl;
        // If the GUI launched the CLI, the user won't have a chance to see
        // message ... the terminal will disappear when the program finishes.
        //  So go in an infinite loop to make sure they see the message.
        select(0,NULL,NULL,NULL,NULL);
        retval = -1;
    }
    ENDTRY

#ifdef WIN32
    // cleanup allocs that were necessary for windows (runFile may be used in argv_py_style)
    if(runFile !=0)
        delete [] runFile;
#endif

    // Delete the argv2 array.
    delete [] argv2;
    delete [] argv_after_s;
    delete [] argv_py_style;

    return retval;
}
