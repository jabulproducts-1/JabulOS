import http.server
import socketserver
import os
import subprocess
from pathlib import Path

PORT = 8080
PROJECT_DIR = Path(__file__).resolve().parent.parent
ASSETS_DIR = PROJECT_DIR / "assets"
TOOLS_DIR = PROJECT_DIR / "tools"
DISK_IMAGE = PROJECT_DIR / "build" / "disk.img"

class DeployHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/update.html':
            self.path = '/assets/update.html'
        return super().do_GET()

    def do_POST(self):
        if self.path == '/deploy':
            # In a real implementation, we'd use a multipart parser.
            # For simplicity, we'll try to extract the 'name' from the form data if possible,
            # but since we are using a mock server, we will just look for the most recent ISO
            # and use a default name if not provided.
            print("Deployment signal received from Dashboard.")
            
            try:
                inbox = PROJECT_DIR / "updates" / "inbox"
                isos = list(inbox.glob("*.iso"))
                if isos:
                    latest_iso = max(isos, key=os.path.getmtime)
                    
                    # We can't easily parse multipart without a library here, 
                    # so we'll assume the name is passed in some way or just use the ISO name.
                    # For this task, we will try to read the content to find the name field.
                    content_length = int(self.headers['Content-Length'])
                    post_data = self.rfile.read(content_length)
                    
                    name = latest_iso.stem
                    if b'name="' in post_data:
                        try:
                            parts = post_data.split(b'name="name"')
                            if len(parts) > 1:
                                name_part = parts[1].split(b'\r\n\r\n')[1].split(b'\r\n')[0].decode('utf-8')
                                if name_part:
                                    name = name_part
                        except:
                            pass

                    cmd = [
                        "python3", str(TOOLS_DIR / "update_service.py"),
                        "stage",
                        "--disk", str(DISK_IMAGE),
                        "--iso", str(latest_iso),
                        "--skip-rebuild"
                    ]
                    # We might need to pass the name to update_service.py if it supported it.
                    # Since it doesn't, we will manually update the header after staging.
                    subprocess.run(cmd, check=True)
                    
                    # Manually update the label in the disk header
                    self.update_disk_label(name)
                    
                    self.send_response(200)
                    self.end_headers()
                    self.wfile.write(b"Deployed successfully")
                else:
                    self.send_response(400)
                    self.end_headers()
                    self.wfile.write(b"No ISO found in updates/inbox/")
            except Exception as e:
                print(f"Error during deployment: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(str(e).encode())
        elif self.path == '/remove-update':
            try:
                print("Removal signal received from Dashboard.")
                self.clear_disk_header()
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"Update removed successfully")
            except Exception as e:
                print(f"Error during removal: {e}")
                self.send_response(500)
                self.end_headers()
                self.wfile.write(str(e).encode())
        else:
            self.send_response(404)
            self.end_headers()

    def update_disk_label(self, name):
        if not DISK_IMAGE.exists():
            return
        
        disk_size = DISK_IMAGE.stat().st_size
        header_lba = (disk_size // 512) - 1
        
        with DISK_IMAGE.open("r+b") as f:
            f.seek(header_lba * 512 + 8 + 4 + 4 + 4 + 4 + 4) # Skip Magic(8), Version(4), K_LBA(4), K_Size(4), I_LBA(4), I_Size(4)
            label_bytes = name.encode("ascii", "replace")[:47].ljust(48, b"\x00")
            f.write(label_bytes)
            f.flush()

    def clear_disk_header(self):
        if not DISK_IMAGE.exists():
            return
        
        disk_size = DISK_IMAGE.stat().st_size
        header_lba = (disk_size // 512) - 1
        
        with DISK_IMAGE.open("r+b") as f:
            f.seek(header_lba * 512)
            f.write(b"\x00" * 512)
            f.flush()

if __name__ == "__main__":
    os.chdir(PROJECT_DIR)
    with socketserver.TCPServer(("", PORT), DeployHandler) as httpd:
        print(f"Global Update Dashboard running at http://localhost:{PORT}")
        print("Use 'Developer Access' (PIN: 28200) to deploy updates.")
        httpd.serve_forever()
