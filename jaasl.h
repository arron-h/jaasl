/*!
 The MIT License (MIT)

 Copyright (c) 2013 Arron Hartley
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#ifndef JAASL_H
#define JAASL_H

struct AAsset;
class JAASoundLibPrivate;
class JAASoundLib
{
public:
	JAASoundLib();
	~JAASoundLib();
	
	/*!
		@function initialise
		@brief Intiailises the sound library by calling the relevent OpenSL functions.
		@return true if successful.
	*/
	bool initialise();

	/*!
	 @function destroy
	 @brief Destroys the sound library, freeing memory and hardware handles.
	 */
	void destroy();
	
	/*!
		@function loadFromAsset
		@brief Initialises an audio player object based on the Android Asset file
			descriptor. The library handle is returned in the second paramater.
			If successful the handle will be > 0.
		@param asset The android asset retrieved from the native asset manager.
		@param handle A handle to this audio file.
		@return true if successful.
	*/
	bool loadFromAsset(AAsset* asset, unsigned int& handle);
	
	/*!
		@function play
		@brief Attempts to play the given handle.
		@param handle The audio file handle to play. Retrieved via loadFromAsset.
		@return true if successful.
	*/
	bool play(unsigned int handle);
	
	/*!
		@function stop
		@brief Attempts to stop playback of the given handle.
		@param handle The audio file handle to stop.
		@return true if successful.
	*/
	bool stop(unsigned int handle);

	/*!
		@function pause
		@brief Attempts to pause playback of the given handle.
		@param handle The audio file handle to pause.
		@return true if successful.
	 */
	bool pause(unsigned int handle);
	
	/*!
		@function stopAll
		@brief Attempts to stop playback of all tracks loaded thus far.
	*/
	void stopAll();
	
	/*!
		@function setVolume
		@brief Sets the volume of the given handle. Volume is in 'gain', which is
			normalised between 0.0f and 1.0f.
		@param handle The audio file to set the volume upon.
		@param gain Normalised floating point volume level.
	*/
	void setVolume(unsigned int handle, float gain);

	/*!
		@function setLooped
		@brief Toggles file looping on or off.
		@param bLoop Loop or not.
	 */
	void setLooped(unsigned int handle, bool bLoop);
	
	/*!
		@function setPlayPosition
		@brief Seeks the audio file to the given msec. Seek prefers fast rather than
			accurate method.
		@param handle The audio file to seek.
		@param msec Milliseconds to set the seek to.
	*/
	void setPlayPosition(unsigned int handle, unsigned long msec);
	
	/*!
		@function playLength
		@brief Returns the length of the audio file in milliseconds.
		@param handle The audio file to query.
		@return the audio file length in milliseconds.
	*/
	unsigned long playLength(unsigned int handle);
	
private:
	JAASoundLibPrivate*           m_dptr;
	friend class JAASoundLibPrivate;
};

#endif //JAASL_H
