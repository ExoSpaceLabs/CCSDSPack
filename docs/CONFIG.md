# Configuration file

[../](README.md) - CCSDSPack Documentation


The configuration file can be used to load data into the application clearly. The library is required 
to be installed, and CCSDSPack.h to included in your project to parse the configuration file.

## Data
The supported data types and syntax to be used are described below.

Data types:
- `bool` — `true` or `false`
- `int` — decimal or hexadecimal integer
- `float` — floating-point number
- `string` — text value
- `bytes` — array of integers (decimal or hex)

NOTE: All lines that start with `#` will be ignored by the parser, therefore can be used as comments. 
Integer values can be inserted in both decimal `255` and hexadecimal `0xFF` format.


Syntax:
```ini
<variable_name>:<data_type>=<value>
```

Examples:

```ini
# String 
stringValue:string="Awesome string to test"

# Integer
integerValue:int=42

# Integer Hex
integerValue:int=0x6b

# boolean
booleanValue:bool=true

# float
floatValue:float=0.85

# arrays
buffer:bytes=[ 1, 2, 3, 4, 5 ]
bufferHex:bytes=[ 0xFF, 0xaa, 0xBB, 0xcc, 0xEE ]
bufferHex2:bytes=[ 0x2, 0x40, 0x56, 0x87, 0xf0 ]

# comment  this line is ignored by the parser
# and this line as well 
```

## Usage
The configuration file can be loaded into the application using the following code snippet:
```c++
Config cfg;
std::string configFile = "/path/to/myConfigFile.cfg"
{
    if (auto res = cfg.load(configFile); !res.has_value()) {
      std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
      return res.error().code();
    }
}
```
Where the load member function loads and parses the configuration file. In case of an error, it is 
returned using the Result error management class.

After successfully loading the configuration file, the data can be accessed using:
```c++
if (cfg.isKey("data_field_size")) {
    uint16_t dataFieldSize;
    
    // get data of type Result<T>
    const auto res = cfg.get<int>("data_field_size");
    
    // check for error
    if (!res.has_value()) {
        std::cerr <<  res.error().message() << std::endl;
        return res.error().code();
    }else{
        // assign retrieved value
        dataFieldSize = res.value();
    }
    
    // alternatively using the macro which performs the check for the user
    ASSIGN_OR_PRINT(dataFieldSize, cfg.get<int>("data_field_size"));
}
```

## Template
The template can be used to define a packet which defines parameters included automatically during
the generation of new packets. This allows dynamic update of the packet without changing the source code.

General Settings fields:


| Parameter                 | Data Type | Required |
|---------------------------|-----------|----------|
| `data_field_size`         | int       | Yes      |
| `sync_pattern_enable`     | bool      | Yes      |
| `sync_pattern`            | int       | No       |
| `validation_enable`       | bool      | Yes      |

Packet Main header fields:

| Parameter                      | Data Type | Required                            |
|--------------------------------|-----------|-------------------------------------|
| `ccsds_APID`                   | int       | Yes                                 |
| `ccsds_version_number`         | int       | Yes                                 |
| `ccsds_type`                   | bool      | Yes                                 |
| `ccsds_data_field_header_flag` | bool      | Yes                                 |
| `ccsds_segmented`              | bool      | Yes                                 |
| `define_secondary_header`      | bool      | Yes                                 |
| `secondary_header_type`        | string    | if define_secondary_header True     |
| `pus_version`                  | int       | if define_secondary_header True *   |
| `pus_service_type`             | int       | if define_secondary_header True *   |
| `pus_service_sub_type`         | int       | if define_secondary_header True *   |
| `pus_source_id`                | int       | if define_secondary_header True *   |
| `pus_event_id`                 | int       | if define_secondary_header True **  |
| `pus_time_code`                | int       | if define_secondary_header True *** |

*Note: The highlighted parameters are strictly used with secondary headers of type PUS.

**Note: The highlighted parameter is specific to secondary header of type PUS-B.

***Note: The highlighted parameter is strictly specific to secondary headers of type PUS-C.

See templatePacket.cfg for example usage of these parameters and the source files for tester 
for example code usages.
