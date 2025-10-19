function add( a, b)
{
    return a+b
}

function testLocalFunc(cnt)
{
    if( cnt % 10 === 0 )
        mylog.log5("testLocalFunc called with:", cnt);
}