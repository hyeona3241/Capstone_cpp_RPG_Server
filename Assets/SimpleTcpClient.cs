using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class SimpleTcpClient : MonoBehaviour
{
    [Header("Server Endpoint")]
    public string serverIp = "127.0.0.1";
    public int serverPort = 7777;

    private Socket _sock;
    private CancellationTokenSource _cts;

    async void Start()
    {
        _cts = new CancellationTokenSource();

        try
        {
            _sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp)
            {
                NoDelay = true
            };
            // 필요시 KeepAlive도:
            // _sock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.KeepAlive, true);

            var endPoint = new IPEndPoint(IPAddress.Parse(serverIp), serverPort);

            // Connect는 블로킹이므로 스레드 풀에서 실행
            await Task.Run(() => _sock.Connect(endPoint), _cts.Token);
            Debug.Log($"[Client] Connected to {serverIp}:{serverPort}");

            // 가벼운 핸드셰이크 예시 (선택)
            SendHandshake();

            // 수신 루프 (선택)
            _ = Task.Run(RecvLoop, _cts.Token);
        }
        catch (Exception e)
        {
            Debug.LogError($"[Client] Connect failed: {e}");
            Cleanup();
        }
    }

    void SendHandshake()
    {
        try
        {
            var data = Encoding.ASCII.GetBytes("HI");
            _sock.Send(data);
        }
        catch (Exception e)
        {
            Debug.LogError($"[Client] Send failed: {e}");
        }
    }

    async Task RecvLoop()
    {
        var buf = new byte[1024];
        try
        {
            while (!_cts.IsCancellationRequested)
            {
                int n = await Task<int>.Run(() => _sock.Receive(buf), _cts.Token);
                if (n <= 0) break; // 서버가 닫음
                // 서버가 브로드캐스트/에코 해주면 여기서 처리
                Debug.Log($"[Client] Received {n} bytes");
            }
        }
        catch (ObjectDisposedException) { }
        catch (Exception e)
        {
            Debug.LogWarning($"[Client] RecvLoop ended: {e.Message}");
        }
        finally
        {
            Debug.Log("[Client] Disconnected");
            Cleanup();
        }
    }

    void OnApplicationQuit() => Cleanup();

    void Cleanup()
    {
        try { _cts?.Cancel(); } catch { }
        try { _sock?.Shutdown(SocketShutdown.Both); } catch { }
        try { _sock?.Close(); } catch { }
        _sock = null;
    }
}
