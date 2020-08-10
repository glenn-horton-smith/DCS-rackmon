# Instructions for use

This is a template repository for developing EPICS IOCs for Mu2e DCS. It aims to provide a developer everything needed
to write all IOCs to be run on a single PC.

## What's inside?
The template comes with the following items:
- `epics-base` folder containing a checkout of EPICS base (currently v3.15.7).
- `docs` folder for putting documentation for your IOCs.
- `README.md` file to be adapted to describe your new repo.
- `setup_dcs.sh` file to setup environment variables

## Getting Started
### Compiling EPICS
The first step is to compile EPICS:
1. `cd` into `epics-base`
2. Run `make`
This step should only need to be done once, unless you decide to change version of EPICS.

### Setup EPICS environment
After compiling, you will need to add the necessary environment variables EPICS needs to work.
- From top-directory of the template, run `source setup_dcs.sh`

### Update your README!
The README.md file will be copied word-for-word from the template repository. Update to something helpful for people
browsing your code! Some instructions for this are included in [README.md](./README.md).

### Start adding IOCs
You can start adding EPICS IOCs as subdirectories in the top level. This will contain your standard EPICS IOC
directory structure.

### Add documentation
Detailed documentation for what your code does should live inside the `docs` folder. Standard markdown (`.md`) files
are recommended. Instructions and tips can (soon) be found in the `docs` folder.

## Optional settings
### Reflect upstream changes to template
The template is still in active development, although we hope it will be stable soon. If you wish to reflect the
changes made to the template in your local repository, do the following:
1. Run `git remote add upstream https://github.com/mhedges/dcs_template.git`
2. Run `git pull upstream master`
This will pull any changes that have been made to the upstream template into your local repository. It is not
expected that you will have to run this often, but the option is there should you hear of changes made to the
template that you want to pull in locally.

### More coming soon!
We're still developing this repo, so be on the lookout for tools for making `db` files, checking out EPICS device
drivers, and (hopefully) more!
