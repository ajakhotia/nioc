@0xd7472994f9605ebc;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("nioc::terminus");


struct Envelope @0xbbe393db90ef9d35 (Payload)
{
    arrivalTimestamp @0 : Int64;
    # steady_clock nanoseconds since its (process-local) epoch.

    sequenceNumber @1 : UInt64;
    # Producer-assigned monotonic counter; 0 when unassigned. Contiguous numbering lets a reader spot
    # drops or reordering after the fact.

    message @2 : Payload;
    # The carried payload. A null message marks a gap.
}
