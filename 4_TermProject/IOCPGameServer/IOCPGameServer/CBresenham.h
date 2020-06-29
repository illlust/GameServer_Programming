#pragma once
#include <list>

struct POS
{
	int m_ix;
	int m_iy;

	POS(int _x, int _y)
	{
		m_ix = _x;
		m_iy = _y;
	}
	POS() {}
};


class CBresenham
{
public:

	POS startPos;
	POS endPos;

	int addNum;
	int error;
	float errorMax;

	int deltaX;
	int deltaY;

public:
	CBresenham();
	~CBresenham();

	std::list<POS> dotList;

	bool checkDot();
	
	bool compareLength(); //0-°¡·Î±è/1-¼¼·Î±è

	//void LineDraw(HWND hWnd);

	void setPos(int _startPosX, int _startPosY, int _endPosX, int _endPosY);
};

