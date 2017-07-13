

// The ned file for the Identifier-Locator Communications Cache.

package inet.networklayer.ilnp;

// @kbteja..
// This simple module represents the ILCC which is core to the ILNP Protocol.

// Has two gates one input and one output from the ipv6 module.

simple ILCC
{
    parameters:

    gates:
        input fromIPv6;
        output toIPv6;
}
