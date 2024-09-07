# MFFS

Mina's fucking filesystem.

Inspired by FAT and HPFS with "fuck your opinion" in mind.

## Some numbers

- Max number of sectors: `2^32`
- Userdata a sector can hold: `SECTOR_SIZE`
- Max file size in bytes: `((2^32) * SECTOR_SIZE)`

In practice, you have to subtract the reserved sectors from `SECTOR_SIZE`.

## Structure

The Filesystem contains Entries and Data Sectors.
It starts on sector 1 (sector 0 contains boot code) with a root entry containing file entries.
File entries point to raw data while the data from entries that have the `Directory` attribute set will be interpreted as other file entries.

## Directories

As above mentioned, directories are just normal file entries.
They claim a sector to start saving other file entries.

### Root Entry

The root contains filesystem metadata followed by other file entries.

| Name              | Size     | Description                                                 |
| ----------------- | -------- | ----------------------------------------------------------- |
| Magic             | char[3]  | ascii string 'FCK'                                          |
| Sector Size       | uint8_t  | sector size                                                 |
| Name              | char[15] | ascii string containing a (disk) name                       |
| Lock              | uint8_t  | lock preventing write access while a process writes to disk |
| Size              | uint32_t | number of sectors the root dir takes                        |
| Allocation Map    | uint32_t | sector number of the deallocation list                      |
| Latest Allocation | uint32_t | latest allocated sector number                              |

The root directories file entry capacity depends on the block size, since it won't use [sector marker](#sector-marker). (This is because I'm to lazy to change and tbh I think it's just good practice to not bloat a root dir :3). The capacity for 512B Sectors should be 7.

Adding sector markers for the root entry is at least on my TODO list.

#### Sector Size

| Value | Meaning                       |
| ----- | ----------------------------- |
| 0     | one sector is 512 byte large  |
| 1     | one sector is 1024 byte large |
| 2     | one sector is 2048 byte large |
| 3     | one sector is 4096 byte large |

#### Allocation Map

The allocation map is a bitmap with a bit for EVERY FUCKING SECTOR. Oh yeah baby we gonna eat that space up like pizza (i like pizza).

For example:

```
1 1 1 0 0 0
```

6 Sectors. 0, 1 and 2 are reserverd/used and 3, 4 and 5 are free.

As you can see in the example it thw lowest sector number is the MSB.

```
Sec: 0 1 2 3 4 5 6 7  8 9 10 11 12 13 14 15
Bit: 1 1 1 0 0 0 1 0  0 0  0  0  1  1  1  0
```

Just makes sense in my world. Easy to read for humans and not much harder for computers.

Since the map is static and (hopefully) won't change in size, we can stretch it across multiple sectors and just store the size at the start of the first sector.

##### Map Size

The Map starts with 4 bytes storing the size of the map in bytes. So `MAP_SIZE * 8` would be the number of sectors.

##### Structure

| Name          | Size      | Description                                          |
| ------------- | --------- | ---------------------------------------------------- |
| Size          | uint32_t  | Size of the map in bytes                             |
| Eigth Sectors | uint8_t[] | Remaining bytes are 8 sectors ascending from the MSB |

#### Latest Allocation

The latest allocation is... well... the latest allocation. In combination with the Deallocation List we speed up that shit like brrrrr.

Instead of going every time from start to end to find free sectors, we just start at this value.

For example:

```
1 1 1 0 0 0
```

Let's say sector 2 was the last allocated sector (remember, we start counting at 0) and now we want to allocate a new sector. Instead of starting our search at GOD DAMN sector 0 , we start at sector 2 and we will find 3 immediatly.

### File Entry

| Name          | Size     | Description                            |
| ------------- | -------- | -------------------------------------- |
| Name          | char[43] | entry name                             |
| Attributes    | uint8_t  | attribute bitmask                      |
| Size          | uint32_t | number of sectors the entry data takes |
| Start Sector  | uint32_t | start sector number                    |
| Created       | uint32_t | 32bit unix timestamp                   |
| Last Modified | uint32_t | 32bit unix timestamp                   |

A FileEntry name starting with '\0' makes an entry invalid/free and the driver will ignore it.

#### Attributes

| Name      | Description                                                                     |
| --------- | ------------------------------------------------------------------------------- |
| Directory | driver: entry is a directory containing file entries                            |
| Hidden    | userspace: entry is marked as hidden (should be handled by a userspace program) |
| Soft Link | userspace: abstract path in entry links to another entry                        |
| Hard link | driver: abstract path in entry links to another entry                           |
| Code      | userspace: data is executable code                                              |
| System    | driver: entry needs higher priviliges to get modified                           |

## Sector Marker

You will find this piece of shit at the start of every sector. It't a linked list pointing to the sector before and the next sector.

| Name          | Size     | Description                                                               |
| ------------- | -------- | ------------------------------------------------------------------------- |
| Sector Before | uint32_t | The data sector before the current (0 if current is the first)            |
| Sector Next   | uint32_t | The data sector holding the data that comes next (0 if there are no more) |
