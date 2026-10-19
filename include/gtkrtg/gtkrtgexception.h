#ifndef GTK_RTG_EXCEPTION_H
#define GTK_RTG_EXCEPTION_H

#ifdef _WIN32
#pragma warning(disable : 4290)
#endif

#include <gtkrtg/gtkrtgbase.h>
#include <string>

#ifdef GTK_RTG_LOGFILE
#include <fstream>
static std::ofstream logfile("gtkrtg_logged.txt");
#else
#include <iostream>
static std::ostream& logfile = std::cout;
#endif

#define g_throw_if_fail(expr)                             \
	if(expr){}											  \
	else												  \
	{							        				  \
		throw GtkRtgException(#expr, __FILE__, __LINE__); \
	}


class GtkRtgException
{
        std::string expr;
        const char *_FILE_;
        const int   _LINE_;
public:

        GtkRtgException(const char *expr, const char *_FILE_, const int _LINE_)
        : expr(expr), _FILE_(_FILE_), _LINE_(_LINE_) {}

        virtual ~GtkRtgException() {}

        const char *what() const { return expr.c_str(); }

        virtual void trace()
        {
        	logfile << "Condition '" << expr << "'" << " failed, in file: " << _FILE_ << " at line: "<< _LINE_ << "." << std::endl;
        }
};

#endif /* GTK_RTG_EXCEPTION_H */
