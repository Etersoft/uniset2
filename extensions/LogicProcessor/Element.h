#ifndef Element_H_
#define Element_H_
// --------------------------------------------------------------------------
#include <string>
#include <list>
#include <ostream>
#include "Exceptions.h"
// --------------------------------------------------------------------------

class LogicException:
    public UniSetTypes::Exception
{
    public:
        LogicException():UniSetTypes::Exception("LogicException"){}
        LogicException(std::string err):UniSetTypes::Exception(err){}
};


class Element
{
    public:

        typedef std::string ElementID;
        static const ElementID DefaultElementID;
        
        enum InputType
        {
            unknown,
            external,
            internal
        };

        Element( ElementID id ):myid(id){};
        virtual ~Element(){};


         /*!< функция вызываемая мастером для элементов, которым требуется 
             работа во времени.
             По умолчанию ничего не делает.
         */
        virtual void tick(){}

        virtual void setIn( int num, bool state ) = 0;
        virtual bool getOut() = 0;


        inline ElementID getId(){ return myid; }
        virtual std::string getType(){ return "?type?"; }

        virtual Element* find( ElementID id );

        virtual void addChildOut( Element* el, int in_num );
        virtual void delChildOut( Element* el );
        inline int outCount(){ return outs.size(); }

        virtual void addInput( int num, bool state=false );
        virtual void delInput( int num );
        inline  int inCount(){ return ins.size(); }

        friend std::ostream& operator<<(std::ostream& os, Element& el )
        {
            return os << el.getType() << "(" << el.getId() << ")";
        }

        friend std::ostream& operator<<(std::ostream& os, Element* el )
        {
            return os << (*el);
        }
    
    protected: 
        Element():myid(DefaultElementID){}; // нельзя создать элемент без id
    
        struct ChildInfo
        {
            ChildInfo(Element* e, int n):
                el(e),num(n){}
            ChildInfo():el(0),num(0){}
            
            Element* el;
            int num;
        };

        typedef std::list<ChildInfo> OutputList;
        OutputList outs;
        virtual void setChildOut();


        struct InputInfo
        {
            InputInfo():num(0),state(false),type(unknown){}
            InputInfo(int n, bool s): num(n),state(s),type(unknown){}
            int num;
            bool state;
            InputType type;
        };

        typedef std::list<InputInfo> InputList;
        InputList ins;
        
        ElementID myid;
        
    private:

        
};
// ---------------------------------------------------------------------------
class TOR:
    public Element
{

    public:
        TOR( ElementID id, int numbers=0, bool st=false );
        virtual ~TOR();
    
        virtual void setIn( int num, bool state );
        virtual bool getOut(){ return myout; }
        
        virtual std::string getType(){ return "OR"; }
    
    protected:
        TOR():myout(false){}
        bool myout;


    private:
};
// ---------------------------------------------------------------------------
class TAND:
    public TOR
{

    public:
        TAND( ElementID id, int numbers=0, bool st=false );
        virtual ~TAND();
    
        virtual void setIn( int num, bool state );
        virtual std::string getType(){ return "AND"; }
    
    protected:
        TAND(){}

    private:
};

// ---------------------------------------------------------------------------
// элемент с одним входом и выходом
class TNOT:
    public Element
{

    public:
        TNOT( ElementID id, bool out_default );
        virtual ~TNOT();
    
        virtual bool getOut(){ return myout; }
    
        /* num игнорируется, т.к. элемент с одним входом
         */
        virtual void setIn( int num, bool state );
        virtual std::string getType(){ return "NOT"; }
        virtual void addInput( int num, bool state=false ){}
        virtual void delInput( int num ){}
    
    protected:
        TNOT():myout(false){}
        bool myout;

    private:
};

// ---------------------------------------------------------------------------
#endif
