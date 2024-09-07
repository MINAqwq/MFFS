# CLI Toool

The MFFS CLI Tool can be used as some kind of userspace driver. It can read and write to MFFS formatted blockdevices.

## Usage

The CLI Tool can do one intruction per execute, to guarantee a better usage in shell script.

It takes one argument, the arguments parameter and then the blockdevice to operate on.

To provide a seperation between the host vfs and the MFFS path the MFFS path needs to be prefixed with a `m:`.

So a move from host vfs to the mffs formatted blockdevice would look like this:
```sh
./mffscli -m /host/path/file m:/file blockdev
```
