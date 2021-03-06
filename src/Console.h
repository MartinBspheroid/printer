#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#include <ctype.h>              // toupper, isprint
#include <math.h>               // sqrtf, fabsf, fmodf, powf, cosf, sinf, floorf, ceilf
#include <stdio.h>              // vsnprintf, sscanf, printf
#include <functional>
#ifdef _MSC_VER
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#define snprintf _snprintf
#endif


// Play it nice with Windows users. Notepad in 2015 still doesn't display text data with Unix-style \n.
#ifdef _WIN32
#define IM_NEWLINE "\r\n"
#else
#define IM_NEWLINE "\n"
#endif

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))
#define IM_PRINTFARGS(FMT)

// For the console example, here we are using a more C++ like approach of declaring a class to hold the data and the functions.
struct ConsoleWindow
{
	char                  InputBuf[256];
	ImVector<char*>       Items;
	bool                  ScrollToBottom;
	ImVector<char*>       History;
	int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
	ImVector<const char*> Commands;
	
	ConsoleWindow()
	{
		ClearLog();
		HistoryPos = -1;
		Commands.push_back("HELP");
		Commands.push_back("HISTORY");
		Commands.push_back("CLEAR");
		Commands.push_back("CLASSIFY");  // "classify" is here to provide an example of "C"+[tab] completing to "CL" and displaying matches.
		Commands.push_back("line()");
	}
	~ConsoleWindow()
	{
		ClearLog();
		for (int i = 0; i < Items.Size; i++)
			free(History[i]);
	}

	void    ClearLog()
	{
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}
	void AddLog(std::string msg){
		AddLog(msg.c_str());
	}
	void    AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.push_back(strdup(buf));
		ScrollToBottom = true;
	}
	void AddCallback(std::function < void(std::string)> callback){

		std::cout << "Handler added..." << std::endl;
		passCallback = callback;
		
	}

	std::function<void(std::string)> passCallback;

	void Draw(const char* title, bool *opened)
	{
		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin(title, opened))
		{
			ImGui::End();
			return;
		}

		//ImGui::TextWrapped("This example implements a console with basic coloring, completion and history. A more elaborate implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
		//ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

		// TODO: display items starting from the bottom

		
		
		if (ImGui::SmallButton("Clear")) ClearLog();
		//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		static ImGuiTextFilter filter;
		filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
		ImGui::PopStyleVar();
		ImGui::Separator();

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
		// If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		for (int i = 0; i < Items.Size; i++)
		{
			const char* item = Items[i];
			if (!filter.PassFilter(item))
				continue;
			ImVec4 col = ImColor(255, 255, 255); // A better implementation may store a type per-item. For the sample let's just parse the text.
			if (strstr(item, "[error]")) col = ImColor(255, 100, 100);
			if (strstr(item, "ERR")) col = ImColor(255, 0, 0);
			if (strstr(item, "WARN")) col = ImColor(255, 255, 0);
			if (strstr(item, "->")) col = ImColor(100, 100, 255);
			else if (strncmp(item, "# ", 2) == 0) col = ImColor(255, 200, 150);
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(item);
			ImGui::PopStyleColor();
		}
		if (ScrollToBottom)
			ImGui::SetScrollHere();
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* input_end = InputBuf + strlen(InputBuf);
			while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
			if (InputBuf[0])
				ExecCommand(InputBuf);
			strcpy(InputBuf, "");
		}

		// Demonstrate keeping auto focus on the input box
		if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1); // Auto focus

		ImGui::End();
	}

	static int Stricmp(const char* str1, const char* str2)               { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
	static int Strnicmp(const char* str1, const char* str2, int count)   { int d = 0; while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; } return d; }

	void    ExecCommand(const char* command_line)
	{
		AddLog("# %s\n", command_line);
		
		// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
		HistoryPos = -1;
		for (int i = History.Size - 1; i >= 0; i--)	
		if (Stricmp(History[i], command_line) == 0)
		{
			free(History[i]);
			History.erase(History.begin() + i);
			break;
		}
		History.push_back(strdup(command_line));

		// Process command
		if (Stricmp(command_line, "CLEAR") == 0)
		{
			ClearLog();
		}
		else if (Stricmp(command_line, "HELP") == 0)
		{
			AddLog("Commands:");
			for (int i = 0; i < Commands.Size; i++)
				AddLog("- %s", Commands[i]);
		}
		else if (Stricmp(command_line, "HISTORY") == 0)
		{
			for (int i = History.Size >= 10 ? History.Size - 10 : 0; i < History.Size; i++)
				AddLog("%3d: %s\n", i, History[i]);
		}
		else
		{
			passCallback(std::string(command_line));
		}
	}

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	{
		ConsoleWindow* console = (ConsoleWindow*)data->UserData;
		return console->TextEditCallback(data);
	}

	int     TextEditCallback(ImGuiTextEditCallbackData* data)
	{
		AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
		switch (data->EventFlag)
		{
		case ImGuiInputTextFlags_CallbackCompletion:
		{
													   // Example of TEXT COMPLETION

													   // Locate beginning of current word
													   const char* word_end = data->Buf + data->CursorPos;
													   const char* word_start = word_end;
													   while (word_start > data->Buf)
													   {
														   const char c = word_start[-1];
														   if (c == ' ' || c == '\t' || c == ',' || c == ';')
															   break;
														   word_start--;
													   }

													   // Build a list of candidates
													   ImVector<const char*> candidates;
													   for (int i = 0; i < Commands.Size; i++)
													   if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
														   candidates.push_back(Commands[i]);

													   if (candidates.Size == 0)
													   {
														   // No match
														   AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
													   }
													   else if (candidates.Size == 1)
													   {
														   // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
														   data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
														   data->InsertChars(data->CursorPos, candidates[0]);
														   data->InsertChars(data->CursorPos, " ");
													   }
													   else
													   {
														   // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
														   int match_len = (int)(word_end - word_start);
														   for (;;)
														   {
															   int c = 0;
															   bool all_candidates_matches = true;
															   for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
															   if (i == 0)
																   c = toupper(candidates[i][match_len]);
															   else if (c != toupper(candidates[i][match_len]))
																   all_candidates_matches = false;
															   if (!all_candidates_matches)
																   break;
															   match_len++;
														   }

														   if (match_len > 0)
														   {
															   data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
															   data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
														   }

														   // List matches
														   AddLog("Possible matches:\n");
														   for (int i = 0; i < candidates.Size; i++)
															   AddLog("- %s\n", candidates[i]);
													   }

													   break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
													// Example of HISTORY
													const int prev_history_pos = HistoryPos;
													if (data->EventKey == ImGuiKey_UpArrow)
													{
														if (HistoryPos == -1)
															HistoryPos = History.Size - 1;
														else if (HistoryPos > 0)
															HistoryPos--;
													}
													else if (data->EventKey == ImGuiKey_DownArrow)
													{
														if (HistoryPos != -1)
														if (++HistoryPos >= History.Size)
															HistoryPos = -1;
													}

													// A better implementation would preserve the data on the current input line along with cursor position.
													if (prev_history_pos != HistoryPos)
													{
														snprintf(data->Buf, data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
														data->BufDirty = true;
														data->CursorPos = data->SelectionStart = data->SelectionEnd = (int)strlen(data->Buf);
													}
		}
		case ImGuiInputTextFlags_EnterReturnsTrue:
		{
													 data->Buf;
													 passCallback(std::string(data->Buf));
													 std::cout << data->Buf << std::endl;
		}
		}
		return 0;
	}
};

static void ShowExampleAppConsole(bool* opened)
{
	static ConsoleWindow console;
	console.Draw("Console", opened);
}