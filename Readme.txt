Diamond Rio PMP300 FS-Plugin v0.2b for Total Commander 5.5 and newer
====================================================================

Highly experimental port of The Snowblind Alliance RIO utility v1.07
to Total Commander FS-Plugin interface. At this moment, it is able to:

1. Work under Windows 9x/NT/2k/XP & Total Commander 5.5 and newer.
2. List Rio PMP300 MP3 Player main (NOT extended yet!) flash memory.
3. Delete files from Rio PMP300 MP3 Player.
4. Download files from Rio PMP300 MP3 Player.
5. Upload files to Rio PMP300 MP3 Player.

Use at your own risk!!!

Please note that this is NOT a front-end like my previous XRio project
(http://sysd.org/proj/hard.php#xrio), thus it has different requirements
than original RIO utility.


 * Requirements:
----------------

DriverLINX Port I/O Driver - http://sysd.org/proj/runtime/port95nt.exe
(despite it's name says it's for Windows 95/NT, it works on ANY Windows).


 * How to install this plugin (32 bit only):
--------------------------------------------

1. Unzip rio.wfx & rio.cfg files to Total Commander directory
2. Choose 'Configuration => Options => Operation => FS-Plugins'
3. Click "Add"
4. Choose rio.wfx
5. Click OK. You can now access the plugin in "Network Neighborhood"
6. Open rio.cfg file and set the correct LPT port address.


* ChangeLog:
------------

 - v0.01a
	* first public release


* TODO:
-------

 o Add more verbosity.
 o Simplify configuration.
 o Support extended memory.
 o Support PMP500 player.


 * References:
--------------

The Snowblind Alliance (http://www.world.co.uk/sba/)
	- made original RIO utility.

Scientific Software Tools, Inc. (http://www.sstnet.com/)
	- made DriverLINX Port I/O Driver for Win95 and WinNT.


 * Author:
----------

Stanislaw Y. Pusep (a.k.a. Stas)

    E-Mail:	cr@cker.com.br
    Site:	http://sysd.org/
(here you can find this program & other cool things)
