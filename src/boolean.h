#pragma once

class Boolean
{
private:
	bool b;
	bool s;
public:
	/* constructor */
	Boolean ()
	{
		b = false;
		s = false;
	}
	
	/* copy constructors */
	Boolean (const Boolean &copy)
	{
		b = copy.b;
		s = copy.s;
	}
	Boolean (const bool &copy)
	{
		b = copy;
		s = true;
	}
	
	/* assignment */
	Boolean& operator=(const Boolean &assignment)
	{
		b = assignment.b;
		s = assignment.s;
		return *this;
	}
	Boolean& operator=(const bool assignment)
	{
		b = assignment;
		s = true;
		return *this;
	}

	/* cast operator */
	operator bool() const
	{
		return b;
	}
	
	/* has b been set */
	bool is_set() const
	{
		return s;
	}
};

