@0xd7472994f9605ebc;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::chronicle");

using Timestamp = import "/nioc/common/idl/timestamp.capnp".Timestamp;


struct Frame @0xbbe393db90ef9d35 (Msg)
{
    arrivalTimestamp @0 : Timestamp;
    # Timestamp corresponding to the moment when the system
    # first perceived this data. Please note that this in not
    # the timestamp of the message itself as it may be affected
    # by the transmission, scheduling, and processing delays.

    message @1 : Msg;
    # The message.
}
