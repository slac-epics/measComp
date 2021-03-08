# measComp: Measurement Computing Release Notes

## Release 2-6 (December 11, 2020)
  - mcBoard\_E-1608.cpp (Linux)
      - Increased the timeout waiting for the module to respond from 1
        second to 3 seconds. This seems to have completely fixed errors
        on the units in the APS Vibration Measurement System. Previously
        there were errors when polling the digital I/O bits and other
        operations, and AInScanStart\_E1608 would sometimes require more
        than 5 retries.
      - Bug fix where lock was not taken when it should be.
  - E-1608::AInScanStart\_E1608 (Linux)
      - Changed the number of retries from 5 to 50. The APS Vibration
        Measurement System was occasionally failing even after 5
        retries. However, after increasing the timeout from 1 to 3
        seconds (see above) they have observed at most 1 retry, so this
        change may not be necessary.
  - iocBoot
      - Previously each iocBoot/\* directory had its own copy of
        save\_restore.cmd, and they were all identical. Changed to using
        a single copy in iocBoot/save\_restore.cmd.
  - scaler record support
      - The support for the scaler record has been moved from the std
        module to the scaler module. This required changes to a number
        of the files in measComp.

## Release 2-5-1 (November 20, 2020)
  - drvMultiFunction.cpp
      - Fixed bug in waveform digitizer mode, current channel was being
        calculated incorrrectly.
      - Improved poller to reduce the number of error messages when the
        device is not available.
  - E-1608::AInScanStart\_E1608 (Linux)
      - Added 5 retries to start waveform acquisition. It appears that
        sometimes the connect() has not actually completed before
        acquisition is started, and the device returns a "not ready"
        status.
      - Improved error messages when acquisition does not start
        correctly.
      - Bug fix for 1-byte message not being sent correctly.
  - mcBoard\_E-1608.cpp (Linux)
      - Bug fix where lock was not taken when it should be.

## Release 2-5 (November 9, 2020)
  - Improvements to USB-CTR04/08 support
      - Added support for reading the 8 digital I/O bits in the MCS
        scan. This allows capturing the status of input and output bits
        during a scan. There are now 9 MCS spectra: 8 for the counters
        and 1 for the digital I/O.
      - Added ability to select counters to use in the MCS on an
        individual basis, rather than the first and last counter to use.
        This also applies to the digital I/O word.
      - Added an AbsTimeWF waveform record containing the absolute time
        of each point in waveform digitizer and MCS scans.
      - Added support for cbDaqInScan and cbDaqSetTrigger on Linux.
      - Aded TrigMode record to control the trigger mode of DaqInScan.
      - Fixed bugs on Linux when the number of counters selected was not
        a power of 2. The data could be wrong and it would stop before
        the number of channels to collect was reached.
  - Added support for E-DIO24 module on Linux and Windows .
  - Added autoconverted OPI files for edm, caQtDM, CSS-Boy and
    CSS-Phoebus.
  - Updated the version of the Measurement Computing Windows UL library
    from 6.51 to 6.72.

## Release 2-4 (September 14, 2019)
  - Improvements to USB-CTR04/08 support
      - Added support for Linux.
      - Added Dwell\_RBV record to show the actual dwell time which can
        be different from requested.
      - Added Point0Action record to control how the first time point in
        an MCS scan is handled. Choices are "Clear", "No clear", and
        "Skip".
      - Fixed bugs when FirstCounter\!=0 or LastCounter\!=7.
      - Fixed initialization bug which was causing random crashes at
        startup.
      - Improved update rate at long dwell times.

## Release 2-3 (August 9, 2019)
  - Improved behavior for E-1608 when there are brief network outages.
    If the waveform digitizer was running when the outage occurred it
    would stop and not restart when the network recovered. This has been
    fixed, and tested with the E-1608 on both Linux and Windows.
  - Added Makefile to produce autoconverted OPI files from medm to edm,
    CSS/BOY, and caQtDM. Added/updated the autoconverted files for
    these.

## Release 2-2 (October 4, 2018)
  - Modified Ethernet support on Linux to be tolerant of brief network
    outages. Previously it was not flushing stale input before sending a
    command and reading the response. This could result in a stale
    response being read, rather than the response to the current
    command. If the network cable was briefly disconnected, for example,
    the driver would never recover. Now the driver does recover if the
    cable is briefly disconnected. If the device is not reachable for a
    longer time the OS will close the socket, and the code does not yet
    handle this.
  - Updated to latest version of Warren Jasper's Linux drivers, and
    applied patches. Pushed patches back to his repository where he has
    merged them, including the change to the Ethernet code described
    above.
  - Added support for E-TC32 to measComp library on Linux. Previously it
    was not supported due to a conflict with E-TC.h.
  - Fixed bug in Linux support for E-TC32. It was not passing the
    channel number correctly when reading the temperature.
  - Fixed bug in Linux support for E-1608. It was not releasing mutex
    when it should, error introduced in R2-1.

## Release 2-1-1 (August 29, 2018)
  - Fixed bug in E-1608 driver, it was not creating epicsEvent in
    constructor.

## Release 2-1 (June 28, 2018)
  - Changed from using std::thread and std::mutex to using epicsThread,
    epicsMutex, and epicsEvent from the EPICS libCom library. This
    allows it to build on older compilers, and is easier to understand.
  - Rearranged code directories.
      - Split linuxSrc into cbw\_linuxSrc (which contains the cbw
        library emulation for Linux) and LinuxDrivers (which contains
        Warren Jasper's Linux drivers).
      - Moved cbw.h, cbw32.lib, and cbw64.lib from src/ into new
        directory measCompSupport.
  - Fixed problem with uninitialized variables in E-TC and E-TC32
    drivers on Linux. This could lead to failures to connect to the
    devices at iocInit.

## Release 2-0 (March 22, 2018)

  - Added support for Linux. Previously measComp was limited to running
    on Windows because it uses the Measurement Computing Universal
    Library (UL), which is available only on Windows. The Linux support
    is designed as follows:
      - It uses the \[low-level Linux drivers from Warren
        Jasper\](https://github.com/wjasper/Linux\_Drivers).
      - On top of these drivers the module provides a layer that
        emulates the Measurement Computing Windows UL library.
      - The EPICS drivers thus always use the UL API and are identical
        on Linux and Windows.
      - The Linux UL layer is independent of EPICS, and uses std::thread
        and std::mutex to provide the required threading and mutex
        capabilities. These methods require C++11, and so will not build
        with very old compilers. They do build with gcc 4.8.5 (e.g. RHEL
        7/Centos 7), and gcc 4.4.7 (e.g. RHEL 6/Centos 6).
      - Currently only the E-1608, E-TC, and E-TC32 models are supported
        on Linux. Support for other modules is straightforward to add
        and can be done as the need arises.
    R2-0 is backwards compatible with previous releases but was given a
    new major release number because of all the new code that was added
    to support Linux.
  - Added support to drvMultiFunction for the E-TC. This is an Ethernet
    module with 8 thermocouple inputs (type B, E, J, K, N, R, S, T),
    digital I/O, and a counter. Added new medm screen ETC\_module.adl
    and new iocBoot directory, iocETC. This device is useful in
    applications where the length limitation of USB and/or the ability
    to move around without an attached computer are important. This
    module has Linux support.
  - Improved the support for the E-1608 to include fast analog input,
    i.e. waveform digitizer support. Also added Linux support for this
    module.
  - Added Linux support for the TC-32 module using Ethernet. This is not
    yet tested.

## Release 1-5 (March 2, 2018)
  - Added support to drvMultiFunction for the E-1608. This is an
    Ethernet module with analog input, analog output, digital I/O, and a
    counter. Added new medm screen E1608\_module.adl and new iocBoot
    directory, iocE1608. This device is useful in applications where the
    length limitation of USB and/or the ability to move around without
    an attached computer are important.
  - Renamed 1608GCounter.template to measCompCounter.template, since it
    is a generic file.
  - Changed line endings in all .substitutions files to be Linux.

## Release 1-4 (September 18, 2017)
  - Added support to drvMultiFunction for the USB-1608G. This is similar
    to the previously supported USB-1608GX-2A0 except that is is 250 kHz
    vs 500 kHz, and does not have the 2 analog output ports.
  - Added support for new version of USB-1608GX-2A0 which is
    functionally the same but has a different board type identifier.
  - Renamed the iocBoot/iocUSB1608G to iocUSB1608G-2A0, and added new
    iocUSB1608 directory for the USB-1608G.
  - Added support to drvMultiFunction for the USB-231, and a new
    iocUSB231 directory.
  - Removed parameter counting and NUM\_PARAMS argument to
    asynPortDriver constructor in all drivers. These are not needed in
    asyn R4-32.

## Release 1-3-1 (May 12, 2016)
  - Added support to drvMultiFunction for the USB-1208FS.
  - Fixed problem with measCompTemperatureSetup.adl medm screen for
    TC-32.

## Release 1-3 (May 10, 2016)
  - Added support to drvMultiFunction for the TC-32, which is a
    32-channel thermocouple input module with both USB and Ethernet
    interfaces.
  - Added new iocBoot/iocTC32 directory.
  - Updated from release 6.50 to 6.52 of the Measurement Computing
    Universal Library.

## Release 1-2 (February 11, 2016)
  - Added support for the USB-1208LS and USB-2408-2A0.
  - Renamed the drv1608G driver to drvMultiFunction because it is now
    designed to handle any multi-function model. It currently supports
    the USB-1208LS, USB-2408-2A0, and USB-1608GX-2A0.
  - Updated from release 6.3 to 6.5 of the Measurement Computing
    Universal Library.

## Release 1-1 (June 26, 2014)
  - Added support for the new USB-CTR04/08 counter/timer modules. This
    includes support for the EPICS scaler record (similar to the Joerger
    VSC and SIS3820), and multi-channel scaler mode (similar to the
    SIS3820). This device can replace the SIS3820 in most applications
    for $429, which is less than 10% of the cost of the SIS3820.
  - Updated from release 6.1 to 6.3 of the Measurement Computing
    Universal Library.
  - Added dependency on synApps mca module for USB-CTR08 driver.
  - Added more fields to the measCompAnalogIn\_settings.req and
    measCompAnalogOut\_settings.req files for save/restore.
  - Added \_RBV (readback) fields for pulse generator Period, Frequency,
    Width, and Delay since the requested value may not match the actual
    value due to hardware limitations.
  - Add new demoSrc directory that contains a tutorial/demo on how to
    write a driver using asynPortDriver. The example is based on the
    USB-1608GX-2AO. It includes:
      - 6 versions of the demo driver source code, starting with a very
        simple 132 line driver that just does 2 analog output records,
        and ending with the full driver that is 1255 lines and has
        waveform digitizer and waveform generator support, etc.
      - A new iocMeasCompDemo directory with the various versions of the
        startup script.
      - New medm files (measCompDemoTop.adl, 1608G\_V\[1,2,3,4,5\].adl).
      - New files in the documentation directory, measCompDriverTalk.pdf
        and measCompTutorial.pdf that were presented at an EPICS class
        on writing drivers based on asynPortDriver.
  - Added the Run record to measCompPulseGen\_settings.req, so it
    remembers the run/stop state.

## Release 1-0 (Nov. 28, 2011)
  - First release. Has support for USB-4303 and USB-1608GX-2AO.

-----

Suggestions and Comments to:  
[Mark Rivers](mailto:rivers@cars.uchicago.edu) :
(rivers@cars.uchicago.edu)