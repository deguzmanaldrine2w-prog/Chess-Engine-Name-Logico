
#pragma once

#include <cliext/vector>

using namespace System;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::Text::RegularExpressions;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace ChessStarter
{
    public ref class Form1 : public Form
    {
    private:
        TextBox^ txtEnginePath;
        TextBox^ txtMoves;
        TextBox^ txtOutput;
        NumericUpDown^ numDepth;

        Button^ btnStart;
        Button^ btnAnalyze;
        Button^ btnStop;
        Button^ btnNewGame;

        Label^ lblStatus;
        Process^ engine;

        bool searchRunning;

    public:
        Form1()
        {
            engine = nullptr;
            searchRunning = false;

            InitializeInterface();

            this->FormClosing +=
                gcnew FormClosingEventHandler(
                    this, &Form1::HandleClosing);
        }

    private:
        void InitializeInterface()
        {
            this->Text = "C++ Chess Engine - Stockfish UCI";
            this->ClientSize = Drawing::Size(850, 620);
            this->StartPosition = FormStartPosition::CenterScreen;
            this->MinimumSize = Drawing::Size(700, 500);

            Label^ lblPath = gcnew Label();
            lblPath->Text = "Stockfish executable:";
            lblPath->Location = Point(15, 18);
            lblPath->AutoSize = true;

            txtEnginePath = gcnew TextBox();
            txtEnginePath->Location = Point(155, 15);
            txtEnginePath->Width = 670;
            txtEnginePath->Anchor =
                AnchorStyles::Top |
                AnchorStyles::Left |
                AnchorStyles::Right;
            txtEnginePath->Text =
                Path::Combine(
                    Application::StartupPath,
                    "stockfish.exe");

            btnStart = gcnew Button();
            btnStart->Text = "Start Engine";
            btnStart->Location = Point(15, 52);
            btnStart->Width = 120;
            btnStart->Click +=
                gcnew EventHandler(this, &Form1::StartEngine);

            btnNewGame = gcnew Button();
            btnNewGame->Text = "New Game";
            btnNewGame->Location = Point(145, 52);
            btnNewGame->Width = 100;
            btnNewGame->Click +=
                gcnew EventHandler(this, &Form1::NewGame);

            Label^ lblMoves = gcnew Label();
            lblMoves->Text = "Moves (UCI):";
            lblMoves->Location = Point(15, 100);
            lblMoves->AutoSize = true;

            txtMoves = gcnew TextBox();
            txtMoves->Location = Point(155, 97);
            txtMoves->Width = 670;
            txtMoves->Anchor =
                AnchorStyles::Top |
                AnchorStyles::Left |
                AnchorStyles::Right;
            txtMoves->Text = "e2e4 e7e5 g1f3";

            Label^ lblDepth = gcnew Label();
            lblDepth->Text = "Search depth:";
            lblDepth->Location = Point(15, 140);
            lblDepth->AutoSize = true;

            numDepth = gcnew NumericUpDown();
            numDepth->Location = Point(155, 137);
            numDepth->Minimum = 1;
            numDepth->Maximum = 40;
            numDepth->Value = 12;
            numDepth->Width = 70;

            btnAnalyze = gcnew Button();
            btnAnalyze->Text = "Analyze";
            btnAnalyze->Location = Point(245, 135);
            btnAnalyze->Width = 100;
            btnAnalyze->Click +=
                gcnew EventHandler(this, &Form1::Analyze);

            btnStop = gcnew Button();
            btnStop->Text = "Stop Search";
            btnStop->Location = Point(355, 135);
            btnStop->Width = 110;
            btnStop->Click +=
                gcnew EventHandler(this, &Form1::StopSearch);

            lblStatus = gcnew Label();
            lblStatus->Text = "Engine not started";
            lblStatus->Location = Point(15, 180);
            lblStatus->AutoSize = true;

            txtOutput = gcnew TextBox();
            txtOutput->Location = Point(15, 210);
            txtOutput->Size = Drawing::Size(810, 390);
            txtOutput->Multiline = true;
            txtOutput->ReadOnly = true;
            txtOutput->ScrollBars = ScrollBars::Vertical;
            txtOutput->WordWrap = false;
            txtOutput->Font =
                gcnew Drawing::Font("Consolas", 10);
            txtOutput->Anchor =
                AnchorStyles::Top |
                AnchorStyles::Bottom |
                AnchorStyles::Left |
                AnchorStyles::Right;

            this->Controls->Add(lblPath);
            this->Controls->Add(txtEnginePath);
            this->Controls->Add(btnStart);
            this->Controls->Add(btnNewGame);
            this->Controls->Add(lblMoves);
            this->Controls->Add(txtMoves);
            this->Controls->Add(lblDepth);
            this->Controls->Add(numDepth);
            this->Controls->Add(btnAnalyze);
            this->Controls->Add(btnStop);
            this->Controls->Add(lblStatus);
            this->Controls->Add(txtOutput);
        }

        void AppendOutput(String^ line)
        {
            if (this->IsDisposed)
                return;

            txtOutput->AppendText(
                line + Environment::NewLine);
        }

        void OnEngineOutput(
            Object^ sender,
            DataReceivedEventArgs^ e)
        {
            if (e->Data == nullptr)
                return;

            // The process output event is asynchronous.
            // Marshal UI updates to the Windows Forms thread.
            if (this->IsDisposed ||
                !this->IsHandleCreated)
                return;

            try
            {
                this->BeginInvoke(
                    gcnew Action<String^>(
                        this, &Form1::AppendOutput),
                    e->Data);
            }
            catch (InvalidOperationException^)
            {
                // The form may be closing.
            }

            if (e->Data->StartsWith(
                    "bestmove ",
                    StringComparison::Ordinal))
            {
                try
                {
                    this->BeginInvoke(
                        gcnew Action(
                            this, &Form1::SearchFinished));
                }
                catch (InvalidOperationException^)
                {
                    // The form may be closing.
                }
            }
        }

        void SearchFinished()
        {
            searchRunning = false;
            lblStatus->Text = "Search finished";
        }

        void SendCommand(String^ command)
        {
            if (engine == nullptr ||
                engine->HasExited)
            {
                throw gcnew InvalidOperationException(
                    "Start the engine first.");
            }

            engine->StandardInput->WriteLine(command);
            engine->StandardInput->Flush();

            AppendOutput("> " + command);
        }

        void StartEngine(Object^ sender, EventArgs^ e)
        {
            if (engine != nullptr && !engine->HasExited)
            {
                MessageBox::Show("The engine is already running.");
                return;
            }

            String^ path = txtEnginePath->Text->Trim();

            if (!File::Exists(path))
            {
                MessageBox::Show(
                    "Stockfish executable not found:\n" + path,
                    "Engine error",
                    MessageBoxButtons::OK,
                    MessageBoxIcon::Error);
                return;
            }

            try
            {
                ProcessStartInfo^ info =
                    gcnew ProcessStartInfo();

                info->FileName = path;
                info->WorkingDirectory =
                    Path::GetDirectoryName(path);
                info->UseShellExecute = false;
                info->CreateNoWindow = true;
                info->RedirectStandardInput = true;
                info->RedirectStandardOutput = true;
                info->RedirectStandardError = true;

                engine = gcnew Process();
                engine->StartInfo = info;
                engine->EnableRaisingEvents = true;

                engine->OutputDataReceived +=
                    gcnew DataReceivedEventHandler(
                        this, &Form1::OnEngineOutput);

                engine->ErrorDataReceived +=
                    gcnew DataReceivedEventHandler(
                        this, &Form1::OnEngineError);

                engine->Start();
                engine->BeginOutputReadLine();
                engine->BeginErrorReadLine();

                // Negotiate the UCI protocol.
                SendCommand("uci");
                SendCommand("isready");

                lblStatus->Text = "Starting Stockfish...";
            }
            catch (Exception^ ex)
            {
                lblStatus->Text = "Engine startup failed";
                AppendOutput("ERROR: " + ex->Message);
            }
        }

        void OnEngineError(
            Object^ sender,
            DataReceivedEventArgs^ e)
        {
            if (e->Data == nullptr ||
                this->IsDisposed ||
                !this->IsHandleCreated)
                return;

            try
            {
                this->BeginInvoke(
                    gcnew Action<String^>(
                        this, &Form1::AppendOutput),
                    "[stderr] " + e->Data);
            }
            catch (InvalidOperationException^)
            {
                // The form may be closing.
            }
        }

        void Analyze(Object^ sender, EventArgs^ e)
        {
            if (engine == nullptr || engine->HasExited)
            {
                MessageBox::Show("Start the engine first.");
                return;
            }

            if (searchRunning)
            {
                MessageBox::Show(
                    "Stop the current search before starting another.");
                return;
            }

            String^ moves = txtMoves->Text->Trim();

            // Validate UCI move notation syntax.
            // This does not validate whether moves are legal.
            if (moves->Length > 0)
            {
                array<String^>^ tokens =
                    Regex::Split(moves, "\\s+");

                for each (String^ move in tokens)
                {
                    if (!Regex::IsMatch(
                            move,
                            "^[a-h][1-8][a-h][1-8][qrbn]?$"))
                    {
                        MessageBox::Show(
                            "Invalid UCI move notation: " + move +
                            "\nExample: e2e4 or a7a8q");
                        return;
                    }
                }
            }

            try
            {
                // Position must be set before searching.
                if (String::IsNullOrWhiteSpace(moves))
                    SendCommand("position startpos");
                else
                    SendCommand("position startpos moves " + moves);

                SendCommand(
                    "go depth " +
                    numDepth->Value.ToString());

                searchRunning = true;
                lblStatus->Text = "Searching...";
            }
            catch (Exception^ ex)
            {
                AppendOutput("ERROR: " + ex->Message);
                lblStatus->Text = "Analysis failed";
            }
        }

        void StopSearch(Object^ sender, EventArgs^ e)
        {
            if (engine == nullptr || engine->HasExited)
                return;

            if (searchRunning)
            {
                try
                {
                    SendCommand("stop");
                    lblStatus->Text = "Stopping search...";
                }
                catch (Exception^ ex)
                {
                    AppendOutput("ERROR: " + ex->Message);
                }
            }
        }

        void NewGame(Object^ sender, EventArgs^ e)
        {
            if (engine == nullptr || engine->HasExited)
            {
                txtMoves->Clear();
                return;
            }

            if (searchRunning)
            {
                MessageBox::Show(
                    "Stop the search before starting a new game.");
                return;
            }

            try
            {
                SendCommand("ucinewgame");
                SendCommand("isready");
                txtMoves->Clear();
                lblStatus->Text = "New game";
            }
            catch (Exception^ ex)
            {
                AppendOutput("ERROR: " + ex->Message);
            }
        }

        void HandleClosing(
            Object^ sender,
            FormClosingEventArgs^ e)
        {
            if (engine == nullptr)
                return;

            try
            {
                if (!engine->HasExited)
                {
                    engine->StandardInput->WriteLine("quit");
                    engine->StandardInput->Flush();

                    if (!engine->WaitForExit(1500))
                        engine->Kill();
                }
            }
            catch (Exception^)
            {
                // Ignore shutdown errors.
            }
            finally
            {
                delete engine;
                engine = nullptr;
            }
        }
    };
}
