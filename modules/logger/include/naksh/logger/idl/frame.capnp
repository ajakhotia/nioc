@0xd7472994f9605ebc;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("naksh::logger");


struct Timestamp @0xb2cac35f6f565bc8
{
    nanosecondSinceEpoch @0 : Int64;
    reference @1 : Text;
}


struct Frame @0xbbe393db90ef9d35 (Message)
{
    arrivalTimestamp @0 : Timestamp;
    # Timestamp corresponding to the moment when the system
    # first perceived this data. Please note that this in not
    # the timestamp of the message itself as it may be affected
    # by the transmission, scheduling, and processing delays.

    message @1 : Message;
    # The message.
}
