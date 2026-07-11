let lastSendResult = false;

workspace.windowActivated.connect(client => {
    if (!client)
        return;

    const result = client.caption === "Clipboard Popup" && client.resourceName === "plasmashell";
    if (result === lastSendResult)
        return;
    lastSendResult = result;
    callDBus("com.q12.kpos", "/", "com.q12.kpos.WindowWatcher", "WindowActivated", result);
});
