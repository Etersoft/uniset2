#ifndef UExceptions_H_
#define UExceptions_H_
// -------------------------------------------------------------------------- 
struct UException
{
	UException(){}
	UException( const std::string e ):err(e){}
	UException( const char* e ):err( std::string(e)){}
	~UException(){}
	
	const char* getError(){ return err.c_str(); }
	
	std::string err;
};
//---------------------------------------------------------------------------
struct UTimeOut:
	public UException
{
	UTimeOut():UException("UTimeOut"){}
	UTimeOut( const std::string e ):UException(e){}
	~UTimeOut(){}
};
//---------------------------------------------------------------------------
struct USysError:
	public UException
{
	USysError():UException("UTimeOut"){}
	USysError( const std::string e ):UException(e){}
	~USysError(){}
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
