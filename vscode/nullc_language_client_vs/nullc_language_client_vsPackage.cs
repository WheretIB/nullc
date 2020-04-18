using System;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.VisualStudio.Shell;
using Task = System.Threading.Tasks.Task;
using Microsoft.VisualStudio.LanguageServer.Client;
using Microsoft.VisualStudio.Threading;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using Microsoft.VisualStudio.Utilities;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio;
using System.ComponentModel.Design;
using EnvDTE80;
using System.Globalization;
using Newtonsoft.Json;

namespace NullcLanguageClientVs
{
    /// <summary>
    /// This is the class that implements the package exposed by this assembly.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The minimum requirement for a class to be considered a valid package for Visual Studio
    /// is to implement the IVsPackage interface and register itself with the shell.
    /// This package uses the helper classes defined inside the Managed Package Framework (MPF)
    /// to do it: it derives from the Package class that provides the implementation of the
    /// IVsPackage interface and uses the registration attributes defined in the framework to
    /// register itself and its components with the shell. These attributes tell the pkgdef creation
    /// utility what data to put into .pkgdef file.
    /// </para>
    /// <para>
    /// To get loaded into VS, the package must be referred by &lt;Asset Type="Microsoft.VisualStudio.VsPackage" ...&gt; in .vsixmanifest file.
    /// </para>
    /// </remarks>
    [PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
    [Guid(NullcLanguageClientVsPackage.PackageGuidString)]
    [ProvideMenuResource("Menus.ctmenu", 1)]
    public sealed class NullcLanguageClientVsPackage : AsyncPackage
    {
        /// <summary>
        /// NullcLanguageClientVsPackage GUID string.
        /// </summary>
        public const string PackageGuidString = "a179464d-38b7-4986-94bc-8ec0add18ab3";

        public const int CommandId = 0x0100;
        public static readonly Guid CommandSet = new Guid("789965FA-B258-4243-8136-AD618B62F6F2");

        private IServiceProvider ServiceProvider => this;

        #region Package Members

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initialization code that rely on services provided by VisualStudio.
        /// </summary>
        /// <param name="cancellationToken">A cancellation token to monitor for initialization cancellation, which can occur when VS is shutting down.</param>
        /// <param name="progress">A provider for progress updates.</param>
        /// <returns>A task representing the async work of package initialization, or an already completed task if there is none. Do not return null from this method.</returns>
        protected override async Task InitializeAsync(CancellationToken cancellationToken, IProgress<ServiceProgressData> progress)
        {
            // When initialized asynchronously, the current thread may be a background thread at this point.
            // Do any initialization that requires the UI thread after switching to the UI thread.
            await this.JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);

            OleMenuCommandService commandService = ServiceProvider.GetService(typeof(IMenuCommandService)) as OleMenuCommandService;

            if (commandService != null)
            {
                CommandID menuCommandID = new CommandID(CommandSet, CommandId);

                MenuCommand menuItem = new MenuCommand(MenuItemCallback, menuCommandID);

                commandService.AddCommand(menuItem);
            }
        }

        private void MenuItemCallback(object sender, EventArgs e)
        {
            try
            {
                try
                {
                    Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();
                }
                catch (Exception)
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, "Error: Wrong thread context", null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                DTE2 dte = (DTE2)ServiceProvider.GetService(typeof(SDTE));

                if (dte == null)
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, "Error: Extension context is unavailable", null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                string currentFile;

                try
                {
                    currentFile = dte.ActiveDocument.FullName;
                }
                catch (Exception)
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, "Error: Open .nc file to debug", null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                if (!currentFile.EndsWith(".nc"))
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, "Error: Open .nc file to debug", null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                var index = currentFile.LastIndexOfAny(new char[]{ '/', '\\'});

                var drive = Char.ToLower(currentFile[0]);

                currentFile = drive + currentFile.Substring(1);

                if (index < 0)
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, String.Format("Error: Path {0} is not an absolute path", currentFile), null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                string configName = currentFile.Substring(0, index) + "\\nc_launch.json";
                string configData = $"{{ \"type\": \"nullc\", \"request\": \"launch\", \"program\": {JsonConvert.ToString(currentFile)} }}";

                try
                {
                    File.WriteAllText(configName, configData);
                }
                catch (Exception ex)
                {
                    VsShellUtilities.ShowMessageBox(ServiceProvider, String.Format("Error: Failed to create launch configuraiton file {0}. {1}", configName, ex.Message), null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                    return;
                }

                string engineName = "5B6BC5F3-2D50-40EE-97F7-105B6FA4EC49";

                string parameters = FormattableString.Invariant($@"/LaunchJson:""{configName}"" /EngineGuid:""{engineName}""");
                dte.Commands.Raise("0ddba113-7ac1-4c6e-a2ef-dcac3f9e731e", 0x0101, parameters, IntPtr.Zero);

                try
                {
                    File.Delete(configName);
                }
                catch (Exception)
                {
                    // Ignore error
                }
            }
            catch (Exception ex)
            {
                VsShellUtilities.ShowMessageBox(ServiceProvider, String.Format(CultureInfo.CurrentCulture, "Launch failed. Error: {0}", ex.Message), null, OLEMSGICON.OLEMSGICON_WARNING, OLEMSGBUTTON.OLEMSGBUTTON_OK, OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
            }
        }

        #endregion
    }

    [ContentType("nullc")]
    [Export(typeof(ILanguageClient))]
    public class NullcLanguageClient : ILanguageClient, IDisposable
    {
        public string Name => "nullc Language Extension";

        public IEnumerable<string> ConfigurationSections => null;

        public object InitializationOptions => null;

        public IEnumerable<string> FilesToWatch => null;

        public event AsyncEventHandler<EventArgs> StartAsync;
        public event AsyncEventHandler<EventArgs> StopAsync;

        private Process process = null;

        public async Task<Connection> ActivateAsync(CancellationToken token)
        {
            await Task.Yield();

            ProcessStartInfo info = new ProcessStartInfo();
            info.FileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "out", @"nullc_lang_server.exe");

            string defaultModulePath = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "modules");

            info.Arguments = "--module_path=\"" + defaultModulePath + "/\"";

            info.RedirectStandardInput = true;
            info.RedirectStandardOutput = true;

            info.UseShellExecute = false;
            info.CreateNoWindow = true;

            process = new Process();
            process.StartInfo = info;

            if (process.Start())
            {
                return new Connection(process.StandardOutput.BaseStream, process.StandardInput.BaseStream);
            }

            return null;
        }

        public async Task OnLoadedAsync()
        {
            await StartAsync.InvokeAsync(this, EventArgs.Empty);
        }

        public Task OnServerInitializeFailedAsync(Exception e)
        {
            return Task.CompletedTask;
        }

        public Task OnServerInitializedAsync()
        {
            return Task.CompletedTask;
        }

        protected virtual void Dispose(bool disposeManaged)
        {
            if (disposeManaged)
            {
                if (process != null)
                    process.Close();
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }

    public static class NullcContentDefinition
    {
        [Export]
        [Name("nullc")]
        [BaseDefinition(CodeRemoteContentDefinition.CodeRemoteContentTypeName)]
        internal static ContentTypeDefinition NullcContentTypeDefinition = null;

        [Export]
        [FileExtension(".nc")]
        [ContentType("nullc")]
        internal static FileExtensionToContentTypeDefinition NullcFileExtensionDefinition = null;
    }
}
