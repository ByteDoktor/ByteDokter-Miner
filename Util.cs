using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ByteMiner
{
    class Util
    {

        public async void CompileMiner(string arguments)
        {

            string baseDir = AppContext.BaseDirectory;

            using (Process process = new Process())
            {

                process.StartInfo.FileName = $"{baseDir}\\portable_tools\\mingw64\\bin\\gcc.exe";

                string newPath = $"{baseDir};{baseDir}\\portable_tools\\mingw64\\bin;";
                var key = process.StartInfo.EnvironmentVariables.Keys.Cast<string>().FirstOrDefault(k => string.Equals(k, "PATH", StringComparison.OrdinalIgnoreCase));

                if (key != null)
                {
                    process.StartInfo.EnvironmentVariables[key] = newPath + process.StartInfo.EnvironmentVariables[key];
                }
                else
                {
                    process.StartInfo.EnvironmentVariables["PATH"] = newPath;
                }
                process.StartInfo.Arguments = arguments;
                process.StartInfo.WorkingDirectory = baseDir;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardError = false;
                process.Start();


                process.WaitForExit();

            }
        }

        public async void CompileResources(string arguments)
        {

            string baseDir = AppContext.BaseDirectory;

            using (Process process = new Process())
            {

                process.StartInfo.FileName = $"{baseDir}\\portable_tools\\mingw64\\bin\\windres.exe";

                string newPath = $"{baseDir};{baseDir}\\portable_tools\\mingw64\\bin;";
                var key = process.StartInfo.EnvironmentVariables.Keys.Cast<string>().FirstOrDefault(k => string.Equals(k, "PATH", StringComparison.OrdinalIgnoreCase));

                if (key != null)
                {
                    process.StartInfo.EnvironmentVariables[key] = newPath + process.StartInfo.EnvironmentVariables[key];
                }
                else
                {
                    process.StartInfo.EnvironmentVariables["PATH"] = newPath;
                }
                process.StartInfo.Arguments = arguments;
                process.StartInfo.WorkingDirectory = baseDir;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardError = false;
                process.Start();


                process.WaitForExit();

            }
        }

        public async void CreateResource(StringBuilder output, string name, byte[] data)
        {
            if (data.Length == 0)
            {
                output.AppendLine($"long {name}Size = 0;");
                output.AppendLine($"unsigned char {name}[] = {{ }};");
                return;
            }

            //byte[] encryptedData1 = Cipher(data, "LoveAllingUnitofLAWniUUXUUXVVXV");
            //string encodedData = Convert.ToBase64String(encryptedData1);
            //byte[] encryptedData2 = Cipher(Encoding.ASCII.GetBytes(encodedData), "LoveAllingUnitofLAWniUUXUUXVVXV");
            output.AppendLine($"long {name}Size = {data.Length};");
            output.AppendLine($"unsigned char {name}[] = {{ {string.Join(",", data)} }};");
            
        }

        public static byte[] Cipher(byte[] data, string key)
        {
            byte[] result = new byte[data.Length];
            for (int i = 0; i < data.Length; i++)
            {
                result[i] = (byte)(data[i] ^ key[i % key.Length]);
            }
            return result;
        }

    }
}