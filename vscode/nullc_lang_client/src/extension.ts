import * as path from 'path';
import * as vscode from 'vscode';
import { debug, workspace, DebugAdapterDescriptorFactory, ExtensionContext } from 'vscode';

import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient';
import { print } from 'util';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
	const configuration = vscode.workspace.getConfiguration("nullc");

	if(configuration.debug)
		console.log("Activating nullc language client extension");

	let serverModule = context.asAbsolutePath(path.join('out', 'nullc_lang_server.exe'));

	let defaultModulePath = context.asAbsolutePath(path.join('modules'));

	let args = configuration.debug ? ['--debug', '--module_path=' + defaultModulePath + "/"] : ['--module_path=' + defaultModulePath + "/"];

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

	if(configuration.debug)
		console.log("Launching client with arguments: " + args);

	// Launch language client
	client = new LanguageClient('nullc language server', serverOptions, clientOptions);

	client.start();

	// Launch name provider
	if(configuration.debug)
		console.log("Registering extension.nullc.getProgramName");

	context.subscriptions.push(vscode.commands.registerCommand('extension.nullc.getProgramName', config => {
		return vscode.window.showInputBox({
			placeHolder: "Please enter the name of the main nullc module file in the workspace folder",
			value: "main.nc"
		});
	}));

	// Register launch configuration provider
	if(configuration.debug)
		console.log("Registering NullcConfigurationProvider");

	const provider = new NullcConfigurationProvider();

	context.subscriptions.push(debug.registerDebugConfigurationProvider('nullc', provider));

	// Register debugger factory
	if(configuration.debug)
		console.log("Registering NullcDebugAdapterDescriptorFactory");

	const factory = new NullcDebugAdapterDescriptorFactory(context);

	context.subscriptions.push(debug.registerDebugAdapterDescriptorFactory('nullc', factory));

	context.subscriptions.push(factory);
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}

	const configuration = vscode.workspace.getConfiguration("nullc");

	if(configuration.debug)
		console.log("Deactivating nullc language client extension");

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
	private defaultModulePath: string;

	constructor(context: ExtensionContext) {
		this.debuggerModule = context.asAbsolutePath(path.join('out', 'nullc_lang_debugger.exe'));

		this.defaultModulePath = context.asAbsolutePath(path.join('modules'));
	}

	createDebugAdapterDescriptor(session: vscode.DebugSession, executable: vscode.DebugAdapterExecutable | undefined): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
		const configuration = vscode.workspace.getConfiguration("nullc");

		let args = configuration.debug ? ['--debug', '--module_path=' + this.defaultModulePath + "/"] : ['--module_path=' + this.defaultModulePath + "/"];

		if(configuration.debug)
			console.log("Launching client with arguments: " + args);

		let debuggerExecutable: vscode.DebugAdapterExecutable = new vscode.DebugAdapterExecutable(this.debuggerModule, args);

		return debuggerExecutable;
	}

	dispose() {

	}
}
