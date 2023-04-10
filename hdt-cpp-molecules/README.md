# Family partitioning 

We provide two different methods to install family partitioning using Docker and natively (compilation)


# Docker 
1)  First, we build the docker version

```
docker build -t smartkg-creator .
```
2) Second, run the docker and mount the folder that contains the HDT file that you want partition. In the following, we give an example for partitioning a sample graph named friends2023 which you can find under folder test
``` docker run --name familyPartioning -d -v > /host/path/target:/file/path/within/container  smartkg-creator getFamilies -s part_friends-_ -m 100 -e friends /mnt/hdts/friends2023.hdt
```

3) Third, we commit the partitions generated into a new image so that we could debug the results or to export the generated partitions

To do so, you can get the container ID using :

sudo docker ps -a

Then, you can commit the container using the following command:
```
sudo docker commit [CONTAINERID] [NewImageName]
```
to run the new image in an interactive shell
```
sudo docker run -t -i [NewImageName] /bin/bash
```
Also, you can export the files fron the container to the host using the following command:
```
docker cp <CONTAINERID>:/file/path/within/container /host/path/target
```

# Compilation

To compile, go inside /smartKG-creator/hdt-cpp-molecules

## Clean code

make clean

## Compile
./autogen.sh
./configure
make

# Show statistics 

./hdt-cpp-molecules/libhdt/tools/getFamiliesEstimate dataset.hdt

## Statistics Parameters

-S : Activate the option to get only families with a presence of a minimum % of subjects. Recommended for very unstructured datasets (e.g. Dbpedia).
-P <percentage>: set up the % of subjects to limit families with a minimum of the given <percentage>. It requires to activate the -S option. If -S is activate and -P is not specified, the default value is 0.01
-L <percentage>: Setup the percentage for infrequent predicates in % occurrences (the more, the less partitions). Predicates with less % occurrences (over the total number of triples) than the given percentage will be discarded and not considered in the families. The default value is 0.01%.
-H <percentage>: Setup the percentage to cut massive predicates in % occurrences (the more, the more partitions). Predicates with more % ocurrences (over the total number of triples) than the given percentage will be discarded. The default value is 0.1%. 
-q: Activate quick estimation (do not perform grouping)

# Generate families

./hdt-cpp-molecules/libhdt/tools/getFamilies dataset.hdt

## Generation Parameters

-s prefix: Prefix for the splitted families (e.g. part_watdiv.10M_). Mandatory
-e <exportFile.json> Export metadata of families in <exportFile>.json
-S : Activate the option to get only families with a presence of a minimum % of subjects. Recommended for very unstructured datasets (e.g. Dbpedia).
-P <percentage>: set up the % of subjects to limit families with a minimum of the given <percentage>. It requires to activate the -S option. If -S is activate and -P is not specified, the default value is 0.01
-L <percentage>: Setup the percentage for infrequent predicates in % occurrences (the more, the less partitions). Predicates with less % occurrences (over the total number of triples) than the given percentage will be discarded and not considered in the families. The default value is 0.01%.
-H <percentage>: Setup the percentage to cut massive predicates in % occurrences (the more, the more partitions). Predicates with more % ocurrences (over the total number of triples) than the given percentage will be discarded. The default value is 0.1%. 
-i Include infrequent predicates (they are not included by default).


# C++ implementation of the HDT compression format

Header Dictionary Triples (HDT) is a compression format for RDF data
that can also be queried for Triple Patterns.

## Getting Started

### Prerequisites

In order to compile this library, you need to have the following
dependencies installed:

- [GNU Autoconf](https://www.gnu.org/software/autoconf/autoconf.html)

  - `sudo apt install autoconf` on Debian-based distros (e.g., Ubuntu)
  - `sudo dnf install autoconf` on Red Hat-based distros (e.g.,
    Fedora)

- [GNU Libtool](https://www.gnu.org/software/libtool/)

  - `sudo apt install libtool` on Debian-based distros (e.g., Ubuntu)
  - `sudo dnf install libtool` on Red Hat-based distros (e.g., Fedora)

- [GNU zip (gzip)](http://www.zlib.net/) Allows GNU zipped RDF input
  files to be ingested, and allows GNU zipped HDT files to be loaded.

  - `sudo apt install gzip` on Debian-based distros (e.g., Ubuntu)
  - `sudo dnf install gzip` on Red Hat-based distros (e.g., Fedora)

- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
  A helper tool for compiling applications and libraries.

  - `sudo apt install pkg-config` on Debian-based distros (e.g.,
    Ubuntu)
  - `sudo dnf install pkgconf-pkg-config` on Red Hat-based distros
    (e.g., Fedora)

- [Serd v0.28+](https://github.com/drobilla/serd) The default parser
  that is used to process RDF input files.  It supports the N-Quads,
  N-Triples, TriG, and Turtle serialization formats.

  - `sudo apt install libserd-0-0 libserd-dev` on Debian-based distros
    (e.g., Ubuntu)
  - `sudo dnf install serd serd-devel` on Red Hat-based distros (e.g.,
    Fedora)

### Installation

To compile and install, run the following commands under the directory
`hdt-cpp`.  This will also compile and install some handy tools.

```
./autogen.sh
./configure
make -j2
sudo make install
```

### Installation issues

Sometimes, the above instructions do not result in a working HDT
installation.  This section enumerates common issues and their
workaround.

#### `./configure` cannot find Serd

While running `./configure` you get a message similar to the
following:

```
Package 'serd-0', required by 'virtual:world', not found
```

This means that `./configure` cannot find the location of the
`serd-0.pc` file on your computer.  You have to find this location
yourself, e.g., in the following way:

```sh
find /usr/ -name serd-0.pc
```

Once you have found the directory containing the `serd-0.pc` file, you
have to inform the `./configure` script about this location by setting
the following environment variable (where directory
`/usr/local/lib/pkgconfig/` is adapted to your situation):

```sh
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/
```

## Using HDT

After compiling and installing, you can use the handy tools that are
located in `hdt-cpp/libhdt/tools`.  We show some common tasks that can
be performed with these tools.

### RDF-2-HDT: Creating an HDT

HDT files can only be created for standards-compliant RDF input files.
If your input file is not standards-compliant RDF, it is not possible
to create an HDT files out of it.

```
$ ./rdf2hdt data.nt data.hdt
```

### HDT-2-RDF: Exporting an HDT

You can export an HDT file to an RDF file in one of the supported
serialization formats (currently: N-Quads, N-Triples, TriG, and
Turtle).  The default serialization format for exporting is N-Triples.

```
$ ./hdt2rdf data.hdt data.nt
```

### Querying for Triple Patterns

You can issue Triple Pattern (TP) queries in the terminal by
specifying a subject, predicate, and/or object term.  The questions
mark (`?`) denotes an uninstantiated term.  For example, you can
retrieve _all_ the triples by querying for the TP `? ? ?`:

    $ ./hdtSearch data.hdt
    >> ? ? ?
    http://example.org/uri3 http://example.org/predicate3 http://example.org/uri4
    http://example.org/uri3 http://example.org/predicate3 http://example.org/uri5
    http://example.org/uri4 http://example.org/predicate4 http://example.org/uri5
    http://example.org/uri1 http://example.org/predicate1 "literal1"
    http://example.org/uri1 http://example.org/predicate1 "literalA"
    http://example.org/uri1 http://example.org/predicate1 "literalB"
    http://example.org/uri1 http://example.org/predicate1 "literalC"
    http://example.org/uri1 http://example.org/predicate2 http://example.org/uri3
    http://example.org/uri1 http://example.org/predicate2 http://example.org/uriA3
    http://example.org/uri2 http://example.org/predicate1 "literal1"
    9 results shown.

    >> http://example.org/uri3 ? ?
    http://example.org/uri3 http://example.org/predicate3 http://example.org/uri4
    http://example.org/uri3 http://example.org/predicate3 http://example.org/uri5
    2 results shown.

    >> exit

### Exporting the header

The header component of an HDT contains metadata describing the data
contained in the HDT, as well as the creation metadata about the HDT
itself.  The contents of the header can be exported to an N-Triples
file:

```
$ ./hdtInfo data.hdt > header.nt
```

### Replacing the Header

It can be useful to update the header information of an HDT.  This can
be done by generating a new HDT file (`new.hdt`) out of an existing
HDT file (`old.hdt`) and an N-Triples file (`new-header.nt`) that
contains the new header information:

```
$ ./replaceHeader old.hdt new.hdt new-header.nt
```

## Contributing

Contributions are welcome!  Please base your contributions and pull
requests (PRs) on the `develop` branch, and not on the `master`
branch.

## License

`hdt-cpp` is free software licensed as GNU Lesser General Public
License (GPL). See `libhdt/COPYRIGHT`.
