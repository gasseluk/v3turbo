# v3turbo
## What's this?
Smart minds have discovered, that there is an errata in the Haswell Xeon v3 processors which allows us to override its turbo multipliers.
See full featured discussion with all background information on an [AnandTech forum](https://forums.anandtech.com/threads/what-controls-turbo-core-in-xeons.2496647/).

This UEFI driver - like many others around - sets your turbo ratio limits to the maximum single-threaded turbo frequency and locks them. 

## Why ?
There are dozens of Haswell Xeon v3 all core turbo unlock efi drivers. Why has there to be another one?
Because this one is easy to build and modify on your own. There's no dependency to EDK2 which would require some initial effort to set up. Instead [GNU-EFI](https://wiki.osdev.org/GNU-EFI) is used which can be installed using `apt` (at least on Ubuntu).

## Build instructions
- Grab yourself a copy of Ubuntu 20.04 (or any other similar distro).
  - Working with a VM is possible and encouraged.

- Install dependencies:
```
sudo apt update
sudo apt install build-essential gnu-efi git
```
- Clone the repo
```
git clone https://github.com/gasseluk/v3turbo.git
```
- Build the driver
```
cd v3turbo
make
```

### Installation
*Ensure you have gotten rid of the microcode update in your UEFI!*
- Install the driver using an UEFI shell
```
# Test driver first
load <path-to-your\v3.efi>

# No errors here.

# Copy to a permanent location on your EFI drive
# for example with fs4 = USB thumb drive:
cp fs4:\v3.efi fs1:\EFI\Boot\v3.efi
bcfg driver add 0 fs1:\EFI\Boot\v3.efi "turbo"
```

## Credits
- All the guys over on the [AnandTech forum](https://forums.anandtech.com/threads/what-controls-turbo-core-in-xeons.2496647/) sharing their insight on this topic.
- [Freecableguy](https://github.com/freecableguy/v3x4) for sharing his excellent (and somewhat more sophisticated) implementation supporting multiple CPUs
- C_Payne for sharing his [assembly source](https://forums.anandtech.com/threads/what-controls-turbo-core-in-xeons.2496647/post-39007971)
- [OSDev.org](https://wiki.osdev.org/Main_Page) for providing excellent [examples](https://wiki.osdev.org/Inline_Assembly/Examples) for instructions like `rdmsr`, `wrmsr` and `cpuid`.

## Compatibility
**This runs (yet) only on single socket systems**
Tested with an Intel Xeon E5-2698v3.
