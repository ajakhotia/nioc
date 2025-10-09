# Logger Module
Provides API to log sensor data. This module allows for creating of data log that may be used
to play back data as it were seen by the online system. Note that the logging features provided 
by this module not to be confused with console logging meant for notifying the user and printing
console message. Use spdlog for such console logging.

## Log Structure
A log contains the following:
* A subdirectory per channel / type.
  * The subdirectory for a channel / type will be identified using the same identifier as
    that of the channel / type
  * Each of the above directories will contain files of size no more than 100 MB to host 
    the data of a given channel / type in the exact same order as perceived by the system
    along with any necessary meta-data such as arrival timestamps, etc.
    * A single message along with the necessary metadata will be referred to as a frame.
  * Files will be numbered starting at 0 and counting up as new ones are created.
  * **NOTES**:
    * Msg type shall not be overloaded. That is, each message type is to be strictly 
      associated with a unique source. Eg: If two source generate Vec3, then a unique
      message type should be created representing each source that may wrap around the
      Vec3 message if needed so that they are logged to distinct channels.
    * The order of the messages perceived by the system is correlated but not 
      reflective of the chronological order of the data. If chronology of data is important, 
      then its upto the user to define appropriate fields in the message to hold the timestamps.
* A binary manifest file containing the sequence of the data. Note that tracking just the
  channel / type of the message allows us to identify the sequence of the entire log as
  the data for each channel / type are already ordered.
