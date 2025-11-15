/*
 * Copyright (c) 2025 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
// --------------------------------------------------------------------------
#include <sstream>
#include "JSModbusClient.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace uniset;
using namespace ModbusRTU;
// --------------------------------------------------------------------------
JSModbusClient::JSModbusClient()
{
}
// --------------------------------------------------------------------------
JSModbusClient::~JSModbusClient()
{
    disconnect();
}
// --------------------------------------------------------------------------
void JSModbusClient::setLog( const shared_ptr<DebugStream>& l )
{
    log = l;

    if( tcp && log )
        tcp->setLog(log);
}
// --------------------------------------------------------------------------
void JSModbusClient::connectTCP( const std::string& host, int port, timeout_t timeout_msec, bool forceDisconnect )
{
    if( host.empty() )
        throw SystemError("JSModbusClient: host undefined");

    if( port <= 0 )
        throw SystemError("JSModbusClient: invalid port");

    if( !tcp )
    {
        tcp = make_unique<ModbusTCPMaster>();

        if( log )
            tcp->setLog(log);
    }

    reconnectEachRequest = forceDisconnect;
    lastHost = host;
    lastPort = port;
    lastTimeout = timeout_msec > 0 ? timeout_msec : lastTimeout;

    tcp->setForceDisconnect(forceDisconnect);
    tcp->disconnect();

    if( timeout_msec > 0 )
        tcp->setTimeout(timeout_msec);

    if( !tcp->connect(host, port, true) )
    {
        ostringstream err;
        err << "JSModbusClient: connect to " << host << ":" << port << " failed";
        throw SystemError(err.str());
    }
}
// --------------------------------------------------------------------------
void JSModbusClient::disconnect()
{
    if( tcp )
        tcp->disconnect();
}
// --------------------------------------------------------------------------
void JSModbusClient::ensureTCP()
{
    if( !tcp )
        throw SystemError("JSModbusClient: ModbusTCPMaster not initialized");
}
// --------------------------------------------------------------------------
void JSModbusClient::ensureConnection()
{
    ensureTCP();

    if( tcp->isConnection() )
        return;

    if( reconnectEachRequest )
    {
        if( lastHost.empty() || lastPort <= 0 )
            throw SystemError("JSModbusClient: no previous TCP parameters for reconnect");

        if( lastTimeout > 0 )
            tcp->setTimeout(lastTimeout);

        if( !tcp->connect(lastHost, lastPort, true) )
        {
            ostringstream err;
            err << "JSModbusClient: reconnect to " << lastHost << ":" << lastPort << " failed";
            throw SystemError(err.str());
        }

        return;
    }

    throw SystemError("JSModbusClient: TCP connection is not established");
}
// --------------------------------------------------------------------------
ReadCoilRetMessage JSModbusClient::read01( ModbusAddr addr, ModbusData start, ModbusData count )
{
    ensureConnection();
    return tcp->read01(addr, start, count);
}
// --------------------------------------------------------------------------
ReadInputStatusRetMessage JSModbusClient::read02( ModbusAddr addr, ModbusData start, ModbusData count )
{
    ensureConnection();
    return tcp->read02(addr, start, count);
}
// --------------------------------------------------------------------------
ReadOutputRetMessage JSModbusClient::read03( ModbusAddr addr, ModbusData start, ModbusData count )
{
    ensureConnection();
    return tcp->read03(addr, start, count);
}
// --------------------------------------------------------------------------
ReadInputRetMessage JSModbusClient::read04( ModbusAddr addr, ModbusData start, ModbusData count )
{
    ensureConnection();
    return tcp->read04(addr, start, count);
}
// --------------------------------------------------------------------------
ForceSingleCoilRetMessage JSModbusClient::write05( ModbusAddr addr, ModbusData reg, bool state )
{
    ensureConnection();
    return tcp->write05(addr, reg, state);
}
// --------------------------------------------------------------------------
WriteSingleOutputRetMessage JSModbusClient::write06( ModbusAddr addr, ModbusData reg, ModbusData value )
{
    ensureConnection();
    return tcp->write06(addr, reg, value);
}
// --------------------------------------------------------------------------
ForceCoilsRetMessage JSModbusClient::write0F( ModbusAddr addr, ModbusData start,
                                              const vector<uint8_t>& values )
{
    ensureConnection();

    if( values.empty() )
        throw SystemError("JSModbusClient: write0F values is empty");

    ForceCoilsMessage msg(addr, start);

    for( auto v : values )
        msg.addBit(v != 0);

    return tcp->write0F(msg);
}
// --------------------------------------------------------------------------
WriteOutputRetMessage JSModbusClient::write10( ModbusAddr addr, ModbusData start,
                                               const vector<ModbusData>& values )
{
    ensureConnection();

    if( values.empty() )
        throw SystemError("JSModbusClient: write10 values is empty");

    WriteOutputMessage msg(addr, start);

    for( auto v : values )
        msg.addData(v);

    return tcp->write10(msg);
}
// --------------------------------------------------------------------------
DiagnosticRetMessage JSModbusClient::diag08( ModbusAddr addr, DiagnosticsSubFunction subfunc,
                                             ModbusData data )
{
    ensureConnection();
    return tcp->diag08(addr, subfunc, data);
}
// --------------------------------------------------------------------------
MEIMessageRetRDI JSModbusClient::read4314( ModbusAddr addr, ModbusByte devID, ModbusByte objID )
{
    ensureConnection();
    return tcp->read4314(addr, devID, objID);
}
// --------------------------------------------------------------------------
