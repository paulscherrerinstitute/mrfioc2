# mrfioc2

[mrfioc2](https://git.psi.ch/epics_drivers/mrfioc2) is an EPICS device support for the Micro Research Finland ([MRF](http://www.mrf.fi/)) timing system (in short mrfioc2 driver). The mrfioc2 enables us to configure and use the event generators and event receivers in the timing system. It comprises of EPICS device support for MRF timing system and uses [devlib2](https://github.com/epics-modules/devlib2/) with additional kernel modules (eg. PCIe) for communication with the hardware.

This project is a continued development from the original mrfioc2 driver available on [GitHub](https://github.com/epics-modules/mrfioc2)


## Documentation
The documentation is available in the `documentation` folder:

* `evr_manual.pdf` is a manual describing how event receiver works and how to configure it. It also provides some general information on the mrfioc2 driver.
* `tutorial.pdf` contains a set of tutorials that instruct the user on how to set up and use an event receiver. Mind that the tutorials are tailored to PSI users, though the functionality and settings of the event receiver are the same.
* `oldDocs` folder contains the old documentation from the original mrfioc2 driver.
* `doxy` folder contains the generated doxygen documentation. For information on how to generate it, inspect readme in `documentation` folder.

## Quick start (PSI)
To set up an IOC application for EVR we need to set up a startup script and a substitution file matching the timing card form factor. Suitable ones are available in the [`PSI/example`](https://git.psi.ch/epics_drivers/mrfioc2/tree/2.7.11/PSI/example) folder:

* EVG
    * example startup script ([`evg_VME_startup.script`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME_startup.script))
    * example substitution files ([`evg_VME-230.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME-230.subs), [`evg_VME-300.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME-300.subs), [`evg_VME-300-fout.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME-300-fout.subs))
* EVR
    * example startup scripts ([`evr_VME_startup.script`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_VME_startup.script), [`evr_PCIe_startup.script`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_PCIe_startup.script))
    * example substitution files ([`evr_cPCI-230.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_cPCI-230.subs), [`evr_PCIe-300.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_PCIe-300.subs), [`evr_PCIe-300DC.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_PCIe-300DC.subs), [`evr_VME-230.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_VME-230.subs), [`evr_VME-300.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_VME-300.subs))

For example, to set up a basic IOC for use with EVR-VME-300 timing card, user should:

* prepare a switable IOC structure in a `TOP` folder (where `TOP` is your project folder)
* copy [`PSI/example/evr_VME-300.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_VME-300.subs) to `TOP/cfg/EVR0.subs`
* configure parameters of the EVR by setting macros in `TOP/cfg/EVR0.subs`. Individual parameters are described in [`documentation/evr_manual.pdf`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/documentation/evr_manual.pdf), and tutorials for various scenarios are available in [`documentation/tutorial.pdf`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/documentation/tutorial.pdf).
* add the following to your startup script (available in [`PSI/example/evr_VME_startup.script`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evr_VME_startup.script)):
    
        require mrfioc2
    
        ##########################
        #-----! EVR Setup ------!#
        ##########################    
    
        ## The following parameters are available to set up the device. They can either be set as an epics environmental variable, or passed as a macro to the 'runScript' command:
        # The following macros are available to set up the mrfioc2:
        # SYS           is used as a prefix for all records. This macro must be defined by the user!
        # DEVICE        is the event receiver / timing card name. (default: EVR0)
        # EVR_SLOT      is the VME crate slot where EVR is inserted. (default: 3)
        # EVR_MEMOFFSET is the base A32 address (default: 0x3000000)
        # EVR_IRQLINE   is the interrupt level. (default: 0x5)
        # EVR_IRQVECT   is the interrupt vector (default: 0x26)
        # EVR_SUBS      is the path to the substitution file that should be loaded. (default: cfg/$(DEVICE).subs=cfg/EVR0.subs)
        #                The following macros can be used to load example substitution files already available in the mrfioc2 module:
        #                EVR_SUBS=$(mrfioc2_DB)/evr_VME-300.subs      for EVR-VME-300 device series
        #                EVR_SUBS=$(mrfioc2_DB)/evr_VME-230.subs      for EVR-VME-230 device series
    
        runScript $(mrfioc2_DIR)/mrfioc2_evr-VME.cmd, "SYS=MTEST-VME-TIMINGTEST, DEVICE=EVR0, EVR_SLOT=3, EVR_MEMOFFSET=0x3000000, EVR_IRQLINE=0x5"

* use `swit -V` to deploy the IOC
* run the GUI by issuing the following command: `start_EVR.sh -s MTEST-VME-TIMINGTEST`

This example shows how to set up a basic IOC for use with EVG-VME-300 timing card:

* prepare a switable IOC structure in a `TOP` folder (where `TOP` is your project folder)
* copy [`PSI/example/evg_VME-300.subs`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME-300.subs) to `TOP/cfg/EVG0.subs`
* configure parameters of the EVG by setting macros in `TOP/cfg/EVG0.subs`. Parameters are described inside the file.
* add the following to your startup script (available in [`PSI/example/evg_VME_startup.script`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/PSI/example/evg_VME_startup.script)):
        
        require mrfioc2

        ##########################
        #-----! EVG Setup ------!#
        ##########################
        ## The following parameters are available to set up the device. They can either be set as an epics environmental variable, or passed as a macro to the 'runScript' command:
        
        # The following macros are available to set up the mrfioc2:
        # SYS 			 is used as a prefix for all records.
        # DEVICE		 is the event generator / timing card name. (default: EVG0)
        # EVG_SLOT		 is the VME crate slot where the card is inserted. (default: 2)
        # EVG_MEMOFFSET	 is the base A24 address (default: 0x0)
        # EVG_IRQLINE 	 is the interrupt level. (default: 0x2)
        # EVG_IRQVECT 	 is the interrupt vector (default: 0x1)
        # EVG_SUBS       is the path to the substitution file that should be loaded. (default: cfg/$(DEVICE).subs=cfg/EVG0.subs)
        #                The following macros can be used to load example substitution files already available in the mrfioc2 module:
        #                EVG_SUBS=$(mrfioc2_DB)/evg_VME-300.subs      for EVM-VME-300 device series
        #                EVG_SUBS=$(mrfioc2_DB)/evg_VME-300-fout.subs for EVM-VME-300 operating as a fanout
        #                EVG_SUBS=$(mrfioc2_DB)/evg_VME-230.subs      for EVG-VME-230 device series
        # MON-PORTS      will only take effect when using health monitoring as configured in evg_VME-300.subs and evg_VME-300-fout.subs substitution files.
        #                This macro selects which ports (SFP 0 - SFP 8) on the device will be monitored for health status. Macro is used as a binary selection of ports. (default: 0x00 = do not monitor SFPs)
        #                Examples:
        #                  MON-PORTS = 0x00  -> do not monitor SFPs
        #                  MON-PORTS = 0x01  -> monitor SFP 0
        #                  MON-PORTS = 0x02  -> monitor SFP 1
        #                  MON-PORTS = 0x03  -> monitor SFP 0 and SFP 1
        #                  MON-PORTS = 0x04  -> monitor SFP 2
        #                  MON-PORTS = 0x85  -> monitor SFP 0, SFP 2 and SFP 8
        #                  MON-PORTS = 0x80  -> monitor SFP 8
        #
        
        runScript $(mrfioc2_DIR)/mrfioc2_evg-VME.cmd, "SYS=MTEST-VME-TIMINGTEST, DEVICE=EVG0, EVG_SLOT=2, EVG_MEMOFFSET=0x000000, EVG_IRQLINE=0x2, MON-PORTS=0x00"
        

* use `swit -V` to deploy the IOC
* run the GUI by issuing the following command: `start_EVG.sh -s MTEST-VME-TIMINGTEST`

## Using the application
As with any EPICS application, build procedure produces all the necessary database files and an IOC for each architecture built. An example application for the `linux-x86_64` architecture is available in `iocBoot` folder. For more details inspect the [`evr_manual.pdf`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/documentation/evr_manual.pdf) available in the [`documentation`](https://git.psi.ch/epics_drivers/mrfioc2/tree/2.7.11/documentation) folder.

GUIs are available:

* `evgMrmApp/opi/EVG/` contains the caQtDm GUI for event generator (event master)
* `evrMrmApp/opi/EVR/` contains the caQtDm GUI for event receiver and health monitor for EVR.

Each folder contains a readme file which explains how to run the GUIs.

### PSI
Example substitution files and startup scripts are available in the [`PSI/example`](https://git.psi.ch/epics_drivers/mrfioc2/tree/2.7.11/PSI/example) folder. For more details inspect the [`evr_manual.pdf`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/documentation/evr_manual.pdf) and [`tutorial.pdf`](https://git.psi.ch/epics_drivers/mrfioc2/raw/2.7.11/documentation/tutorial.pdf) available in the [`documentation`](https://git.psi.ch/epics_drivers/mrfioc2/tree/2.7.11/documentation) folder.


## Supported hardware

* EVG VME-230: VME-230 form factor event generator. (tested and working)
* EVG VME-300: VME-300 form factor event master / event generator.  (tested, check known issues)
* EVG cPCI-230: cPCI-230 form factor event generator (not tested, no GUI)
* EVR VME-230: VME-230 form factor event receiver. (tested and working)
* EVR VME-300: VME-300 form factor event receiver.  (tested, check known issues)
* EVR PCIe-300: PCIe-300 form factor event receiver. (tested and working with firmware version 7. Not everything works with firmware version 202 - check known issues)
* EVR PCIe-300DC: PCIE-300 form factor event receiver with newer hardware. (check known issues)
* EVR cPCI-230: cPCI-230 form factor event receiver (not tested, no GUI)

mrfioc2 driver supports hardware firmware versions up to and including 202.
Minimal supported firmware version for:

* EVG is 3,
* PCIe form factor EVR is 3,
* VME form factor EVR 4.

## Known issues
* 300-series hardware with firmware version 202:
    * some EVR-PCIe-300 devices with firmware version 202 do not work at 142.876 MHz event clock. They were tested, and were working, at 125 MHz event clock.
    * mapping two output sources to one output was not tested on EVR-PCIe-300 devices.
    * data buffer sending does not work if delay compensation is disabled.
    * in addition to the normal segmented data buffer operation, data buffer interrupt also gets triggered by the control logic that was used in older hardware. This in turn means reduced performance, since data buffer handling is triggered when there is no data buffer update. Awaiting firmware fix.
    * CML: only frequency mode works, since hardware addresses which store patterns are currently not accessible.
    * EVM-VME-300: Input registers for FrontInp0 and FrontInp2 are linked together in hardware. Awaiting firmware fix.
* sending the data buffer upstream does not work on EVR-VME-300 and was not tested on other cards.
* PCIe-300DC cards: no detailed tests were made. Event and data buffer reception seems to work with EVG-VME-300 series. This was tested with event link speed up to 150 MHz.

## Dependencies

- [EPICS base](http://www.aps.anl.gov/epics/base/R3-14/index.php) >= 3.14.8.2
- [devlib2](https://github.com/epics-modules/devlib2/) >=2.6
- [MSI tool]( http://www.aps.anl.gov/epics/extensions/msi/index.php) to expand databases (included in EPICS base >= 3.15.1)

__Optional:__

For building the documentation (for more information inspect readme in `documentation` folder):

* [Inkscape](https://inkscape.org/en/) tool for converting _svg_ image format to _pdf_ format.
* LaTeX environment (pdflatex, bibtex) to build the documentation from latex source to pdf format.
* [doxygen](http://www.stack.nl/~dimitri/doxygen/) for generating documentation from source code


## Building from scratch

The mrfioc2 driver is structured as an ordinary EPICS application. In order to build it from source:

* clone the sources from git repository by running command `git clone git@git.psi.ch:epics_drivers/mrfioc2.git`, which creates a top folder called `mrfioc2`.
* update files in `mrfioc2/configure` folder to match your system, and to include additional libraries to be build together with the driver (eg. set paths in `configure/RELEASE`).
* run `make -f Makefile` in the `mrfioc2` folder.


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

* clone the sources from git repository by running command `git clone git@git.psi.ch:epics_drivers/mrfioc2.git`, which creates a top folder called `mrfioc2`.
* run `make` in the `mrfioc2` folder on the build server.
* run 'make db' to create database files
* to install the driver run `make install` in the `mrfioc2` folder on the build server.

The driver builds as a single library, which can be loaded using `require` to your IOC. Installation process also copies all the necessary support files (eg. templates) to the appropriate module folder. For more options inspect driver.makefile and require documentation available at the PSI wiki.

## Kernel sources
mrfioc2 driver uses a special kernel module for communication with hardware. Sources of the `mrf.ko` kernel module are available in `mrmShared/linux`.

## Authors

* Tom Slejko (tom.slejko@cosylab.com)
* Jure Krašna (jure.krasna@cosylab.com)
* Sašo Skube (saso.skube@cosylab.com)

Original driver developers:

* Michael Davidsaver (mdavidsaver@bnl.gov)
* Jayesh Shah (jshah@bnl.gov)
* Eric Bjorklund (bjorklund@lanl.gov)

Please send an e-mail to saso.skube@cosylab.com if anyone is missing!
