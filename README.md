# mrfioc2

[mrfioc2](https://github.psi.ch/projects/ED/repos/mrfioc2/browse) is an EPICS device support for the Micro Research Finland ([MRF](http://www.mrf.fi/)) timing system (in short mrfioc2 driver). The mrfioc2 enables us to configure and use the event generators and event receivers in the timing system. It comprises of EPICS device support for MRF timing system and uses [devlib2](https://github.com/epics-modules/devlib2/) with additional kernel modules (eg. PCIe) for communication with the hardware.

This project is a continued development from the original mrfioc2 driver available on [GitHub](https://github.com/epics-modules/mrfioc2)


## Documentation
The documentation is available in the `documentation` folder:

* `evr_manual.pdf` is a manual describing how event receiver works and how to configure it. It also provides some general information on the mrfioc2 driver.
* `tutorial.pdf` contains a set of tutorials that instruct the user on how to set up and use an event receiver. Mind that the tutorials are tailored to PSI users, though the functionality and settings of the event receiver are the same.
* `oldDocs` folder contains the old documentation from the original mrfioc2 driver.
* `doxy` folder contains the generated doxygen documentation. For information on how to generate it, inspect readme in `documentation` folder.

Note that the documentation is not yet updated with the latest features from firmware 200+.

## Prerequisites

- [EPICS base](http://www.aps.anl.gov/epics/base/R3-14/index.php) >= 3.14.8.2
- [devlib2](https://github.com/epics-modules/devlib2/) >=2.6
- [MSI tool]( http://www.aps.anl.gov/epics/extensions/msi/index.php) to expand databases (included in EPICS base >= 3.15.1)

__Optional prerequisites:__

For building the documentation (for more information inspect readme in `documentation` folder):

* [Inkscape](https://inkscape.org/en/) tool for converting _svg_ image format to _pdf_ format.
* LaTeX environment (pdflatex, bibtex) to build the documentation from latex source to pdf format.
* [doxygen](http://www.stack.nl/~dimitri/doxygen/) for generating documentation from source code


## Building from scratch

The mrfioc2 driver is structured as an ordinary EPICS application. In order to build it from source:

* clone the sources from git repository by running command `git clone https://skube_s@github.psi.ch/scm/ed/mrfioc2.git`, which creates a top folder called `mrfioc2`.
* update files in `mrfioc2/configure` folder to match your system, and to include additional libraries to be build together with the driver (eg. set paths in `configure/RELEASE`).
* run `make -f Makefile` in the `mrfioc2` folder.

Note, that current version does not build on Windows system, due to unresolved external dependancy issue.

Outputs of the build command are:

* Libraries
    * `libevgMrm.so` is event generator/event master (EVM) library
    * `libevrMrm.so` is event receiver (EVG) library
    * `libmrfCommon.so` is required by EVG and EVM and contains common functions and definitions (eg. device support object model)
    * `libmrmShared.so` contains code shared between EVG and EVM, thus required by both. It also contains __kernel modules__ required by the mrfioc2 driver.

* IOC for each architecture that is build using the above libraries, and any other included from `configure/RELEASE`
* Database files

### PSI
Building the driver on the PSI infrastructure is a bit different, since it leverages the driver.makefile. In order to build it:

* clone the sources from git repository by running command `git clone https://skube_s@github.psi.ch/scm/ed/regdev.git`, which creates a top folder called `mrfioc2`.
* run `make` in the `mrfioc2` folder on the build server.
* run 'make db' to create database files
* to install the driver run `make install` in the `mrfioc2` folder on the build server.

The driver builds as a single library, which can be loaded using `require` to your IOC. Installation process also copies all the necessary support files (eg. templates) to the appropriate module folder. For more options inspect driver.makefile and require documentation available at the PSI wiki.


## Using the application
As with any EPICS application, build procedure produces all the necessary database files and an IOC for each architecture built. An example application for the `linux-x86_64` architecture is available in `iocBoot` folder. For more details inspect the `evr_manual.pdf` available in the `documentation` folder.

GUIs are available:

* `evgMrmApp/opi/EVG/` contains the caQtDm GUI for event generator (event master)
* `evrMrmApp/opi/EVR/` contains the caQtDm GUI for event receiver and health monitor for EVR.

Each folder contains a readme file which explains how to run the GUIs.

### PSI
Example substitution files and startup scripts are available in the `PSI/example` folder. For more details inspect the `evr_manual.pdf` and `tutorial.pdf` available in the `documentation` folder.



## Kernel sources
mrfioc2 driver uses a special kernel module for communication with hardware. Sources of the `mrf.ko` kernel module are available in `mrmShared/linux`.

## Supported hardware

* EVG VME-230: VME-230 form factor event generator.
* EVG VME-300: VME-300 form factor event master (event generator).
* EVR VME-230: VME-230 form factor event receiver.
* EVR VME-300: VME-300 form factor event receiver.
* EVR PCIe-300: PCIe-300 form factor event receiver.
* EVR cPCI-230: cPCI-230 form factor event receiver (except GUI)

mrfioc2 driver supports hardware firmware versions up to and including 201.
Minimal supported version for :

* EVG is 3,
* PCIe form factor EVR is 3,
* VME form factor EVR 4.


## Authors

* Tom Slejko (tom.slejko@cosylab.com)
* Jure Krašna (jure.krasna@cosylab.com)
* Sašo Skube (saso.skube@cosylab.com)

Original driver developers:

* Michael Davidsaver (mdavidsaver@bnl.gov)
* Jayesh Shah (jshah@bnl.gov)
* Eric Bjorklund (bjorklund@lanl.gov)

Please send an e-mail to saso.skube@cosylab.com if anyone is missing!


## Todo
* Known issues (missing interrupts on PCIe due to a race condition between kernel and user space - interrupt enable flag ends up disabled)



