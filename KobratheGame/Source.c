#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>

#define InitialLength 16
#define MaxMapDimensions 40
#define GameTitle "Kobra The Game"

LPCSTR SnakeBit = "1"; // "\xDB";
LPCSTR SpaceBit = " ";
LPCSTR BaitBit = "*";
DWORD WrittenCount = 0;
HANDLE ConsoleHandle;

#define PERR(bSuccess) {if(!(bSuccess)) printf("%s: Error %d on line %d\n", __FILE__, GetLastError(), __LINE__);}
#define PrintBit(x,y,what) PERR( WriteConsoleOutputCharacter( ConsoleHandle, what, 1, ( COORD ) { x, y }, &WrittenCount ) )
#define ClearBit(x,y) PERR( WriteConsoleOutputCharacter( ConsoleHandle, SpaceBit, 1, ( COORD ) { x, y }, &WrittenCount ) )
#define Pow2(x) ((unsigned int) 1 << (x))

typedef enum DirectionsTag {dRight, dUp, dLeft, dDown} Directions;
typedef enum SpeedsTag {
	Speed1 = 1,
	Speed2 = 2,
	Speed3 = 5,
	Speed4 = 10,
	Speed5 = 15,
	Speed6 = 30
} Speeds;

typedef struct KobraTag
{
	size_t size;
	COORD * coords;
	LPCSTR looks;
	Directions heads;
} Kobra;

int GetRandomNumber( int From, int To )
{
	int result = rand( );
	int width = To - From;
	int ReRollLimit = RAND_MAX - RAND_MAX % width;

	while ( result >= ReRollLimit )
		result = rand( );

	return result % width + From;
}

void PlaceBait(COORD csBufferSize, int ** MapState, COORD * BaitCoords)
{
	int RandomNumber = GetRandomNumber( 0, csBufferSize.Y * csBufferSize.X );
	while ( MapState[0][RandomNumber] == 1 )
		RandomNumber = GetRandomNumber( 0, csBufferSize.Y * csBufferSize.X );
	BaitCoords->X = RandomNumber % csBufferSize.Y;
	BaitCoords->Y = RandomNumber / csBufferSize.Y;
	PrintBit( BaitCoords->X, BaitCoords->Y, "+" );
	MapState[BaitCoords->Y][BaitCoords->X] = 1;
}

int main( ) {
	COORD TopLeft = { 0 };
	CONSOLE_SCREEN_BUFFER_INFOEX csBufferInfo = { .cbSize = sizeof csBufferInfo };
	//DWORD ConsoleSize = 0;
	CONSOLE_FONT_INFOEX cFontInfo = { 0 };
	CONSOLE_CURSOR_INFO cCursorInfo = { 0 };
	int ScreenX = 0,
		ScreenY = 0;
	LPCSTR toStart = "Press any direction key to start";
	int toStartSize = 0;
	COORD toStartCoord = { 0 };
	LPCSTR dead = "You are dead";
	int deadSize = 0;
	COORD deadCoord = { 0 };
	Kobra Kobra1 = { 0 };
	int SleepDelay = 16;
	int Speed = Speed5;
	int Limit = 30;
	int BuildUp = 0;
	int HasMoved = 0;
	int initialPause = 1;
	time_t CurrentTime = 0;
	int ** MapState = NULL;
	int GameOver = 0;
	int ReRollLimit = 0;
	COORD BaitCoords = { 0 };
	char * ConsoleTitle = malloc( 64 * sizeof * ConsoleTitle );
	size_t GameTitleLength = strlen( GameTitle " - " );

	//Start making ConsoleTitle
	strcpy( ConsoleTitle, GameTitle " - " );

	//Set ConsoleHandle
	ConsoleHandle = GetStdHandle( STD_OUTPUT_HANDLE );
	if ( ConsoleHandle == INVALID_HANDLE_VALUE )
	{
		printf( "invalid handle: %d", GetLastError( ) );
	}
	if ( ConsoleHandle == NULL )
	{
		printf( "null" );
	}

	//Seed random number generator with the current time
	CurrentTime = time( NULL );
	srand( (unsigned int) CurrentTime );

	//Squares the window dimensions of the console
	PERR( GetConsoleScreenBufferInfoEx( ConsoleHandle, &csBufferInfo ) );
	if ( csBufferInfo.dwSize.X < csBufferInfo.dwSize.Y )
	{
		if ( MaxMapDimensions < csBufferInfo.dwSize.X )
		{
			csBufferInfo.dwSize.X = MaxMapDimensions;
			csBufferInfo.dwSize.Y = MaxMapDimensions;
		}
		else
			csBufferInfo.dwSize.Y = csBufferInfo.dwSize.X;
	}
	else
	{
		if ( MaxMapDimensions < csBufferInfo.dwSize.Y )
		{
			csBufferInfo.dwSize.X = MaxMapDimensions;
			csBufferInfo.dwSize.Y = MaxMapDimensions;
		}
		else
			csBufferInfo.dwSize.X = csBufferInfo.dwSize.Y;
	}
	PERR( SetConsoleScreenBufferInfoEx( ConsoleHandle, &csBufferInfo ) );
	PERR( GetConsoleScreenBufferInfoEx( ConsoleHandle, &csBufferInfo ) );

	//Allocates memory for MapState that will hold the states of the cells
	MapState = malloc( csBufferInfo.dwSize.Y * sizeof * MapState );
	MapState[0] = calloc( csBufferInfo.dwSize.Y * csBufferInfo.dwSize.X, sizeof * MapState[0] );
	for (int i = 1; i < csBufferInfo.dwSize.Y; i++)
		MapState[i] = MapState[i - 1] + csBufferInfo.dwSize.X;

	if ( GetCurrentConsoleFontEx( ConsoleHandle, 1, &cFontInfo ) )
		printf( "%d", cFontInfo.nFont );
	else
		printf( "error: %u", GetLastError( ) );

	//Squares (8 x 8) the character dimensions of the console
	cFontInfo.cbSize = sizeof cFontInfo;
	cFontInfo.nFont = 2;
	cFontInfo.dwFontSize.X = 8;
	cFontInfo.dwFontSize.Y = 8;
	cFontInfo.FontFamily = FF_DONTCARE;
	PERR( SetCurrentConsoleFontEx( ConsoleHandle, 1, &cFontInfo ) );

	//Makes the console cursor invisible
	PERR( GetConsoleCursorInfo( ConsoleHandle, &cCursorInfo ) );
	cCursorInfo.bVisible = 0;
	PERR( SetConsoleCursorInfo( ConsoleHandle, &cCursorInfo ) );

	//Places the console window to the center of the whole screen
	ScreenX = GetSystemMetrics( SM_CXSCREEN ) / 2 - 4 * csBufferInfo.dwSize.X;
	ScreenY = GetSystemMetrics( SM_CYSCREEN ) / 2 - 4 * csBufferInfo.dwSize.Y - 20;
	PERR( SetWindowPos( GetConsoleWindow( ), 0, ScreenX, ScreenY, 1000000, 1000000, SWP_NOZORDER ) );

	//ConsoleSize = csBufferInfo.dwSize.X * csBufferInfo.dwSize.Y;
	//FillConsoleOutputCharacter( ConsoleHandle, ' ', ConsoleSize, TopLeft, &WrittenCount );

	//Initializes the Kobra
	Kobra1.size = InitialLength;
	Kobra1.coords = calloc( Kobra1.size + 1, sizeof * Kobra1.coords );
	Kobra1.looks = "1";
	Kobra1.heads = dRight;
	for ( size_t i = 0; i < Kobra1.size; i++ ) {
		Kobra1.coords[i].X = csBufferInfo.dwSize.X / 2 - i;
		Kobra1.coords[i].Y = csBufferInfo.dwSize.Y / 2;
		PrintBit( Kobra1.coords[i].X, Kobra1.coords[i].Y, Kobra1.looks );

		//Records it on the MapState
		MapState[Kobra1.coords[i].Y][Kobra1.coords[i].X] = 1;
	}
	sprintf( ConsoleTitle + GameTitleLength, "%d", Kobra1.size );
	PERR( SetConsoleTitle( ConsoleTitle ) );

	//Prints guideline on how to start
	while ( toStart[toStartSize] != 0 ) toStartSize++;
	toStartCoord.X = ( csBufferInfo.dwSize.X - toStartSize ) / 2;
	toStartCoord.Y = csBufferInfo.dwSize.Y / 4;
	PERR( WriteConsoleOutputCharacter( ConsoleHandle, toStart, toStartSize, toStartCoord, &WrittenCount ) );

	//Set the ReRollLimit
	ReRollLimit = RAND_MAX - RAND_MAX % (csBufferInfo.dwSize.X * csBufferInfo.dwSize.Y);

	while ( !GameOver ) {

		if ( GetAsyncKeyState( VK_RIGHT ) | GetAsyncKeyState( VK_LEFT ) | GetAsyncKeyState( VK_DOWN ) | GetAsyncKeyState( VK_UP ) ) {
			
			//Clears the guideline
			for ( int i = 0; i < toStartSize; i++ )
				ClearBit( toStartCoord.X + i, toStartCoord.Y );

			//Places a bait to somewhere random on the map
			PlaceBait( csBufferInfo.dwSize, MapState, &BaitCoords );

			while ( 1 ) {
				if ( GetAsyncKeyState( VK_LEFT ) && ( Kobra1.heads % 2 == 1 ) && HasMoved ) {
					Kobra1.heads = dLeft;
					HasMoved = 0;
				}
				if ( GetAsyncKeyState( VK_RIGHT ) && ( Kobra1.heads % 2 == 1 ) && HasMoved ) {
					Kobra1.heads = dRight;
					HasMoved = 0;
				}
				if ( GetAsyncKeyState( VK_UP ) && ( Kobra1.heads % 2 == 0 ) && HasMoved ) {
					Kobra1.heads = dUp;
					HasMoved = 0;
				}
				if ( GetAsyncKeyState( VK_DOWN ) && ( Kobra1.heads % 2 == 0 ) && HasMoved ) {
					Kobra1.heads = dDown;
					HasMoved = 0;
				}

				BuildUp += Speed;

				if ( BuildUp >= Limit ) {
					BuildUp -= Limit;

					for ( size_t i = Kobra1.size; i > 0; i-- )
						Kobra1.coords[i] = Kobra1.coords[i - 1];
					Kobra1.coords[0].X = ( Kobra1.coords[1].X + ( Kobra1.heads == dRight ) - ( Kobra1.heads == dLeft ) + csBufferInfo.dwSize.X ) % csBufferInfo.dwSize.X;
					Kobra1.coords[0].Y = ( Kobra1.coords[1].Y + ( Kobra1.heads == dDown ) - ( Kobra1.heads == dUp ) + csBufferInfo.dwSize.Y ) % csBufferInfo.dwSize.Y;

					if ( MapState[Kobra1.coords[0].Y][Kobra1.coords[0].X] == 1 ) {
						if ( Kobra1.coords[0].X == BaitCoords.X && Kobra1.coords[0].Y == BaitCoords.Y )
						{
							Kobra1.size++;
							Kobra1.coords = realloc( Kobra1.coords, ( Kobra1.size + 1 ) * sizeof * Kobra1.coords );
							sprintf( ConsoleTitle + GameTitleLength, "%d", Kobra1.size );
							PERR( SetConsoleTitle( ConsoleTitle ) );

							PlaceBait( csBufferInfo.dwSize, MapState, &BaitCoords );
						}
						else
						{
							MapState[Kobra1.coords[Kobra1.size].Y][Kobra1.coords[Kobra1.size].X] = 0;
							ClearBit( Kobra1.coords[Kobra1.size].X, Kobra1.coords[Kobra1.size].Y );
							PrintBit( Kobra1.coords[0].X, Kobra1.coords[0].Y, "X" );

							GameOver = 1;
							break;
						}
					}
					else
					{
						MapState[Kobra1.coords[Kobra1.size].Y][Kobra1.coords[Kobra1.size].X] = 0;
						ClearBit( Kobra1.coords[Kobra1.size].X, Kobra1.coords[Kobra1.size].Y );
					}
					
					PrintBit( Kobra1.coords[0].X, Kobra1.coords[0].Y, Kobra1.looks );

					MapState[Kobra1.coords[0].Y][Kobra1.coords[0].X] = 1;
					HasMoved = 1;
				}

				Sleep( SleepDelay );
			}
		}

		Sleep( SleepDelay );
	}

	//Prints the death message
	while ( dead[deadSize] != 0 ) deadSize++;
	deadCoord.X = ( csBufferInfo.dwSize.X - deadSize ) / 2;
	deadCoord.Y = csBufferInfo.dwSize.Y / 4;
	PERR( WriteConsoleOutputCharacter( ConsoleHandle, dead, deadSize, deadCoord, &WrittenCount ) );

	getchar( );
	return 0;
}