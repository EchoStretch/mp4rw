MP4 Read/Write Example
======================

The following is an example showcasing read and write access to mp4.
The provided example will dump the `a53.elf` and mmdsr logs to `/mnt/usb0/mp4`.

Code execution on mp4 may be achieved by writing to the physical address of
an sdbgp context handler. Writes flush the instruction cache so the overwritten
context handler may be immediately used.
