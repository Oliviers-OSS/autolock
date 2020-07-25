/*
 * InnerException.hpp
 *
 *  Created on: Mar 21, 2014
 *      Author: oc
 * Class Exception declaration
 * WARNING: NO INCLUDE ALLOWED INSIDE THIS FILE
 * that may be included more than once
 */

//#ifndef EXCEPTION_HPP_
//#define EXCEPTION_HPP_

class Exception : public std::exception
{
  struct Location
  {
    const char *file;
    unsigned int line;
    const char *function;
    Location(const char *f = 0,unsigned int l=0,const char *fct =0)
      :file(f),line(l),function(fct)
    {
      //WARNING_MSG("Exception from %s in %s:%u",function,file,line);
    }

    operator std::string()
    {
      std::string s;
      sprintf(s,"%s in %s:%u",function,file,line);
      return s;
    }
  } origin;
  int error_code;
  std::string error_message;

public:

  Exception(const int code,const char *format,va_list params)
	__attribute__ ((format (printf, 3, 0))) __attribute__ ((nonnull(3)))
    :error_code(code)
  {
    vsprintf(error_message,format,params);
    WARNING_MSG("Exception: error code = %d, message = %s",error_code,error_message.c_str());
  }

  Exception(const char *filename, const unsigned int line,const char *function
            ,const int code,const char *format,va_list params)
  	  __attribute__ ((format (printf, 6, 0))) __attribute__ ((nonnull(2))) __attribute__ ((nonnull(4))) __attribute__ ((nonnull(6)))
    :origin(filename,line,function)
    ,error_code(code)
  {
    vsprintf(error_message,format,params);
    const std::string location( (std::string)origin);
    WARNING_MSG("Exception from %s: error code = %d, message = %s",location.c_str(),error_code,error_message.c_str());
  }

  Exception(const int code,const char *format,...)
  	  __attribute__ ((format (printf, 3, 4))) __attribute__ ((nonnull(3)))
    :error_code(code)
  {
    va_list params;
    va_start(params,format);
    vsprintf(error_message,format,params);
    va_end(params);
    WARNING_MSG("Exception: error code = %d, message = %s",error_code,error_message.c_str());
  }

  Exception(const char *filename, const unsigned int line,const char *function
            ,const int code,const char *format,...)
  	  __attribute__ ((format (printf, 6, 7))) __attribute__ ((nonnull(2))) __attribute__ ((nonnull(4))) __attribute__ ((nonnull(6)))
    :origin(filename,line,function)
    ,error_code(code)
  {
    va_list params;
    va_start(params,format);
    vsprintf(error_message,format,params);
    va_end(params);
    const std::string location( (std::string)origin);
    WARNING_MSG("Exception from %s: error code = %d, message = %s",location.c_str(),error_code,error_message.c_str());
  }

  Exception(const int code,const std::string &error)
    :error_code(code)
    ,error_message(error)
  {
    WARNING_MSG("Exception: error code = %d, message = %s",error_code,error_message.c_str());
  }

  virtual ~Exception() throw()
  {
  }

  virtual const char* what() const throw()
  {
    return error_message.c_str();
  }

  int code() const
  {
    return error_code;
  }
};

//#endif /* EXCEPTION_HPP_ */
