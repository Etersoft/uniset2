load("uniset2-modbus-client.js");

function runModbusTests(port)
{
    const result = { success: true };

    try
    {
        const client = uniset_createModbusClient();
        client.connectTCP({ host: "127.0.0.1", port: port, forceDisconnect: false });

        const read01 = client.read01({ slave: 1, reg: 0, count: 4 });
        if( !read01 || typeof read01.byteCount !== "number" )
            throw new Error("read01 invalid reply");

        const read02 = client.read02({ slave: 1, reg: 0, count: 4 });
        if( !read02 || typeof read02.byteCount !== "number" )
            throw new Error("read02 invalid reply");

        const read03 = client.read03({ slave: 1, reg: 0, count: 2 });
        if( !read03 || !Array.isArray(read03.values) || read03.values.length < 2 )
            throw new Error("read03 invalid reply");
        if( read03.values[0] !== 0 || read03.values[1] !== 1 )
            throw new Error("read03 unexpected values: " + read03.values);

        const read04 = client.read04({ slave: 1, reg: 0, count: 2 });
        if( !read04 || !Array.isArray(read04.values) || read04.values.length < 2 )
            throw new Error("read04 invalid reply");

        // write05/write06/write0F/write10 – сервер отвечает эхо-значениями, поэтому проверяем поля ack-ответов
        const write05 = client.write05({ slave: 1, reg: 0, value: 1 });
        if( !write05 || typeof write05.start !== "number" || write05.start !== 0
            || typeof write05.value !== "boolean" || write05.value !== true )
        {
            throw new Error("write05 invalid reply: " + JSON.stringify(write05));
        }

        const write06 = client.write06({ slave: 1, reg: 1, value: 123 });
        if( !write06 || typeof write06.start !== "number" || write06.start !== 1
            || typeof write06.value !== "number" || write06.value !== 123 )
        {
            throw new Error("write06 invalid reply: " + JSON.stringify(write06));
        }

        const write0F = client.write0F({ slave: 1, reg: 0, values: [1, 0, 1, 0] });
        if( !write0F || typeof write0F.start !== "number" || write0F.start !== 0
            || typeof write0F.count !== "number" || write0F.count !== 4 )
        {
            throw new Error("write0F invalid reply: " + JSON.stringify(write0F));
        }

        const write10 = client.write10({ slave: 1, reg: 3, values: [11, 22] });
        if( !write10 || typeof write10.start !== "number" || write10.start !== 3
            || typeof write10.count !== "number" || write10.count !== 2 )
        {
            throw new Error("write10 invalid reply: " + JSON.stringify(write10));
        }

        const diag = client.diag08({ slave: 1, subfunc: 0, data: 0xAA });
        if( !diag || typeof diag.count !== "number" )
            throw new Error("diag08 invalid reply");

        const id = client.read4314({ slave: 1, devID: 2, objID: 3 });
        if( !id || !Array.isArray(id.objects) || id.objects.length === 0 )
            throw new Error("read4314 invalid reply");

        client.disconnect();
    }
    catch( e )
    {
        result.success = false;
        result.error = e && e.message ? e.message : "" + e;
    }

    return result;
}
