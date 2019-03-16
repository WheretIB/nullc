import * as path from 'path';
import * as vscode from 'vscode';
import { debug, workspace, DebugAdapterDescriptorFactory, ExtensionContext } from 'vscode';

import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
	let serverModule = context.asAbsolutePath(path.join('out', 'nullc_lang_server.exe'));

	//let args = ['--debug'];
	let args = [];

	let serverOptions: ServerOptions = {
		command: serverModule,
		args: args
	};

	let clientOptions: LanguageClientOptions = {
		documentSelector: [{ scheme: 'file', language: 'nullc' }, { scheme: 'file', pattern: '**/*.nc' }],
		synchronize: {
			fileEvents: workspace.createFileSystemWatcher('**/*.nc')
		},
	};

	// Launch language client
	client = new LanguageClient('nullc language server', serverOptions, clientOptions);

	client.start();

	// Launch name provider
	context.subscriptions.push(vscode.commands.registerCommand('extension.nullc.getProgramName', config => {
		return vscode.window.showInputBox({
			placeHolder: "Please enter the name of the main nullc module file in the workspace folder",
			value: "main.nc"
		});
	}));

	// Register launch configuration provider
	const provider = new NullcConfigurationProvider();

	context.subscriptions.push(debug.registerDebugConfigurationProvider('nullc', provider));

	// Register debugger factory
	const factory = new NullcDebugAdapterDescriptorFactory(context);

	context.subscriptions.push(debug.registerDebugAdapterDescriptorFactory('nullc', factory));

	context.subscriptions.push(factory);
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}

class NullcConfigurationProvider implements vscode.DebugConfigurationProvider {
	resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined, config: vscode.DebugConfiguration, token?: vscode.CancellationToken): vscode.ProviderResult<vscode.DebugConfiguration> {

		// if launch.json is missing or empty
		if (!config.type && !config.request && !config.name) {

			const editor = vscode.window.activeTextEditor;

			const configuration = vscode.workspace.getConfiguration("nullc");

			if (editor && editor.document.languageId === 'nullc') {
				config.type = 'nullc';
				config.name = 'Launch';
				config.request = 'launch';
				config.program = '${file}';
				config.workspaceFolder = '${workspaceFolder}';
				config.modulePath = configuration.module_path;
				config.trace = configuration.trace.server;
			}
		}

		if (!config.program) {
			return vscode.window.showInformationMessage("Cannot find a program to debug").then(_ => {
				return undefined;	// abort launch
			});
		}

		return config;
	}
}

class NullcDebugAdapterDescriptorFactory implements DebugAdapterDescriptorFactory {
	private debuggerModule?: string;

	constructor(context: ExtensionContext) {
		this.debuggerModule = context.asAbsolutePath(path.join('out', 'nullc_lang_debugger.exe'));
	}

	createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable | undefined): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
		//let args = ['--debug'];
		let args = [];

		let debuggerExecutable: vscode.DebugAdapterExecutable = new vscode.DebugAdapterExecutable(this.debuggerModule, args);

		return debuggerExecutable;
	}

	dispose() {

	}
}
