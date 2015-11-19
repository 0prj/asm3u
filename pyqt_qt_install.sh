#!/bin/sh
 
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install python-dev
sudo apt-get install qt4-dev-tools
 
mkdir pyqt
cd pygt
 
echo Install SIP
wget http://sourceforge.net/projects/pyqt/files/sip/sip-4.16.6/sip-4.16.6.tar.gz
tar vxf sip-4.16.6.tar.gz
cd sip-4.16.6
python configure.py
make
sudo make install
cd ..
 
echo Istall PyQt-4.10.4
wget http://sourceforge.net/projects/pyqt/files/PyQt4/PyQt-4.10.4/PyQt-x11-gpl-4.10.4.tar.gz/download
cp download PyQt-x11-gpl-4.10.4.tar.gz  
tar vxf PyQt-x11-gpl-4.10.4.tar.gz
cd PyQt-x11-gpl-4.10.4
python configure.py
make
sudo make install
cd ..
 
echo =========================================
echo QT4 and PyQT 4.10.4/SIP 4.16.6 Installed.
echo =========================================
