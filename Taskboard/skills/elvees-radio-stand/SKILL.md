---
name: elvees-radio-stand
description: Remote test-stand topology and workflow for the Elvees radio manager. Use when building, installing, running, or reviewing the radio manager GUI/runtime on the physical stand. All modifications must happen on the remote machines, never on localhost.
disable-model-invocation: true
---

# Elvees radio stand

When using this skill, all modifications must happen on the remote stand
machines — not on localhost. There are three platforms.

## Main host — `ssh dev@el001407-pc`

The main machine where work is reviewed and the GUI is launched.

- GUI at `/home/dev/radio-manager/gui`, launched via `./gui.sh`.
- The radio manager logic is built and installed at
  `/home/dev/radio-manager/modules/extern/runtime` via `./tools/build.sh` then
  `./tools/install.sh`. The runtime must be installed for the GUI to build
  correctly.
- `/home/dev/radio-manager/modules` — collection of all applications and blocks
  that compile into the package `./build/packages/CoreManager<Test>`.

## USRP-connected host — `ssh dev@el001408-pc`

A Linux machine with a USRP X310 connected to it.

## Mcom03 board — `ssh root@OpenWRT_ASSET-2812-1`

An ARM + two elcores machine.
