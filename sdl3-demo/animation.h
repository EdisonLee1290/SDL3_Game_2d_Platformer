#pragma once
#include "timer.h"

class Animation
{
	Timer timer;
	int frameCount; // number of animation frames in a animation
	int startFrame;
	int rowIndex;
	int framesPerRow;
public:
	Animation() : timer(0), frameCount(0), startFrame(0), rowIndex(0) {}
	Animation(int frameCount, float length, int startFrame, int rowIndex, int framesPerRow = 8) : frameCount(frameCount), timer(length), startFrame(startFrame), rowIndex(rowIndex), framesPerRow(framesPerRow)
	{
	}

	float getLength() const { return timer.getlength(); }
	float getRowIndex() const { return rowIndex; }

	// used to calculate current frame we need to display for the sprite
	int currentFrame() const
	{
		// time value / time length value = % of beginning to end of animation
		// we then must multiply by frameCount to get the frame we need to display on screen
		return static_cast<int>(timer.getTime() / timer.getlength() * frameCount);
	}
	void step(float deltaTime)
	{
		timer.step(deltaTime);
	}
	void reset()
	{
		timer.reset();
	}
};