import * as path from 'path';
import { workspace, ExtensionContext } from 'vscode';

import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient';

let client: LanguageClient;

export function activate(context: ExtensionContext) {
	let serverModule = context.asAbsolutePath(path.join('..', 'nullc_lang_server', 'out', 'nullc_lang_server.exe'));

	let serverOptions: ServerOptions = {
		command: serverModule,
		args: ['--debug']
	};

	let clientOptions: LanguageClientOptions = {
		documentSelector: [{ scheme: 'file', language: 'nullc' }, { scheme: 'file', pattern: '**/*.nc' }],
		synchronize: {
			fileEvents: workspace.createFileSystemWatcher('**/*.nc')
		},
	};

	client = new LanguageClient('nullc language server', serverOptions, clientOptions);

	client.start();
}

export function deactivate(): Thenable<void> | undefined {
	if (!client) {
		return undefined;
	}
	return client.stop();
}
