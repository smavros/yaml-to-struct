## yaml-to-struct
An example of data parsing in C struct from YAML file

### Scope
This code present a way for parsing data, from a YAML format configuration file, in a C data structure, using libyaml.

### Use
As an example we use a configuration file which describes a 2D thin-wire coil. The coil is made from circular loops, each with 4 attributes (radius,x and y position and current flow direction). Also there are 2 global attributes for the coil (current and frequency).


#### Reallocation
Since the number of the mappings inside the sequence of the config.yaml file are not previous known, the data structure is dynamicaly allocated using the user's defined argument of the mappings which are going to be parsed. If the  user denoted number of mappings is not the same with the number of the mapping in the config file then: 

* If the are more mappings in the config.yaml then the parsing stops and a warning message is printed in the stdout. 

* If there are less mapping then the parsing stops and the dynamic memory allocated arrays are reallocated to the lenght of the existing and parsed mappings. Also a warning message is printed in the stdout.

### Additional Information

- [yaml file format](http://yaml.org/) 
- [libyaml library](http://pyyaml.org/wiki/LibYAML)
- a very informative [tutorial](https://www.wpsoftware.net/andrew/pages/libyaml.html)
