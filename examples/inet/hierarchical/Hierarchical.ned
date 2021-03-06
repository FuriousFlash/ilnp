//
// Copyright (C) 2012 Opensim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

package inet.examples.inet.hierarchical;

import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.nodes.ethernet.Eth100G;
import inet.nodes.ethernet.Eth100M;
import inet.nodes.ethernet.Eth1G;
import inet.nodes.ethernet.EtherSwitch;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;


network Hierarchical
{
    parameters:
        int columns = default(3);
        int rows = default(3);
    types:
        module LAN
        {
            parameters:
                int numHosts = default(3);
                @display("i=misc/cloud;is=s");
            gates:
                inout ethg[];
            submodules:
                switch: EtherSwitch {
                    @display("is=s");
                }
                host[numHosts]: StandardHost {
                    @display("is=s");
                }
            connections:
                for i=0..numHosts-1 {
                    host[i].ethg++ <--> Eth100M <--> switch.ethg++;
                }
                for i=0..sizeof(ethg) - 1 {
                    switch.ethg++ <--> ethg++;
                }
        }

        module Area
        {
            parameters:
                int numAreas = default(3);
                @display("i=misc/cloud;is=s");
            gates:
                inout ethg[];
            submodules:
                lan[numAreas]: LAN;
                router[numAreas]: Router {
                    @display("is=s");
                }
            connections:
                for i=0..numAreas-1 {
                    router[i].ethg++ <--> Eth1G <--> router[(i+1)%numAreas].ethg++ if numAreas > 1;
                    router[i].ethg++ <--> Eth1G <--> lan[i].ethg++;
                }
                router[0].ethg++ <--> ethg++;
        }

    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=37,31");
            config = xmldoc("hierarchical.xml");
        }
        router[columns*rows]: Router {
            @display("p=115,103,matrix,$columns;is=s");
        }
        area[columns*rows]: Area;
    connections:
        for i=0..rows-1, for j=0..columns-1 {
            router[i*columns+j].ethg++ <--> Eth100G <--> router[(i+1)*columns+j].ethg++ if i!=rows-1;
            router[i*columns+j].ethg++ <--> Eth100G <--> router[i*columns+j+1].ethg++ if j!=columns-1;
        }
        for i=0..rows*columns-1 {
            router[i].ethg++ <--> Eth100G <--> area[i].ethg++;
        }
}
