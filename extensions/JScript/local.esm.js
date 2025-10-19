import * as Log from "uniset2-log.esm.js";

function add( a, b)
{
    return a+b
}

const mylog = Log.uniset_log_create("local.js", true, true);
export function testLocalFunc(cnt)
{
    if( cnt % 10 === 0 )
        mylog.log5("testLocalFunc called with:", cnt);
}