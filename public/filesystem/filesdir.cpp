
#include "filesdir.h"

namespace filesystem
{

fdir_file::fdir_file( void* hmod, unsigned int key )
{
	if ( fdir.Init( hmod ) )
	{
		ekey = key;
		state = 0;
	}
	else
	{
		state = S_BAD;
	}
}
bool fdir_file::open( hpath file, int mode )
{
	if ( !(state&S_BAD) )
	{
		// Close existing file we're working with
		if ( state&S_GOOD )
			close();
		
		// Cannot write/append!
		if ( (mode&3)==MODE_READ )
		{
			if ( fdir.FindDesc( &desc, file ) )
			{
				state = S_GOOD;
				name = file;
				return true;
			}
		}
	}
	return false;
}
void fdir_file::close()
{
	off = 0;
	state &= S_BAD; // Clear everything but the bad flag
	name.clear();
}
int fdir_file::status() const
{
	return state;
}
bool fdir_file::info( file_info& fi ) const
{
	if ( state&S_GOOD )
	{
		fi.address = name;
		fi.attributes = 0;
		fi.size = desc.SizeOfData;
		return true;
	}
	return false;
}
bool fdir_file::seek( pos_t pos, int origin )
{
	if ( state&S_GOOD )
	{
		int final = static_cast<int>( pos );
	
		// Adjust the offset
		switch ( origin )
		{
		case 0:
			// begin / SEEK_SET / FILE_BEGIN / 0
			break;
		case 1:
			// current / SEEK_CUR / FILE_CURRENT / 1
			final += off;
			break;
		case 2:
			// end / SEEK_END / FILE_END / 2
			final = desc.SizeOfData - final;
			break;
		NODEFAULT;
		}
		// Check offset on validness
		if ( final>=0 && final<=static_cast<int>(desc.SizeOfData) )
		{
			if ( final==static_cast<int>(desc.SizeOfData) ) state |= S_EOF;
			else state &= ~S_EOF;
			off = final;
			return true;
		}
	}
	return false;
}
file::pos_t fdir_file::tell() const
{
	return off;
}
file::size_t fdir_file::read( void* buf, unsigned int size, unsigned int term ) const
{
	fdir_file& self = *const_cast<fdir_file*>( this );

	if ( state&S_GOOD )
	{
		// Check how many bytes we're allowed to read
		unsigned int off = static_cast<unsigned int>(this->off);
		unsigned int end = off + size;
		if ( end>desc.SizeOfData )
		{
			self.state |= S_EOF;
			end = desc.SizeOfData;
		}
		if ( off>=end )
			goto nobytesread;

		// Read bytes
		const char* in = self.fdir.GetData<const char*>( &desc );
		self.fdir.Decrypt( buf, in+off, end-off );//pelite::FilesDecrypt( hdr, ekey, buf, in+off, end-off );

		// Update seek ptr
		self.off = end;

		// Return number of bytes read
		return end-off;
	}
nobytesread:
	return 0;
}
file::size_t fdir_file::write( const void* src, unsigned int size )
{
	// Not supported
	return 0;
}
file::size_t fdir_file::vprintf( const char* fmt, va_list va )
{
	// Not supported
	return 0;
}
void fdir_file::flush()
{
	// Not supported
}

}
