# Building
## Debian-based Linux

Get QT5 development files
```
apt install qtbase5-dev-tools qtbase5-dev libqt5svg5-dev qt5-qmake
```

Use `qmake` and `make` to build

> [!NOTE]
> Tested 2026-06-15 on debian 13 "trixie" with QMake version 3.1 Using Qt version 5.15.15

### Stand Alone Client
```
qmake -o build/SAC/Makefile StandaloneClient/SAC.pro 
make -C build/SAC

# run 
build/SAC/SAC
```


### Raw Data Storage

For debugging, it's useful for the raid simulator binary to neighbor the RDS executable.

```
qmake -o build/RDS/Makefile Client/RDS.pro
make -C build/RDS

qmake -o build/Sim/Makefile Simulator/RaidSimulator.pro
make -C build/Sim
ln -s $PWD/build/Sim/RaidSimulator build/RDS/RaidSimulator.exe

# run 
build/RDS/RDS
```


