using System.IO.MemoryMappedFiles;

namespace ProcessProtect.Core
{
    public class ProtectCore
    {
        const string sharedMemoryPidsName = "{13037560-5DFF-4A0C-A5E1-3C538F7BC0D9}";
        const string sharedMemoryPathName = "{1AE27252-A0C6-4CD4-9FE3-9E04F5C9C453}";
        const int memoryPidsSize = 4 * 1024;
        const int pidsCount = 1024;
        const int memoryPathSize = 1024;
        bool inited = false;
        bool started = false;
        MemoryMappedFile sharedMemoryPids, sharedMemoryPath;
        MemoryMappedViewAccessor pidsAccessor, pathAccessor;
        uint[] pids = new uint[pidsCount];
        string path = new string("");

        public ProtectCore()
        {
            sharedMemoryPids = MemoryMappedFile.CreateNew(sharedMemoryPidsName, memoryPidsSize,MemoryMappedFileAccess.ReadWrite);
            sharedMemoryPath = MemoryMappedFile.CreateNew(sharedMemoryPathName, memoryPathSize,MemoryMappedFileAccess.ReadWrite);
            pidsAccessor = sharedMemoryPids.CreateViewAccessor();
            pathAccessor = sharedMemoryPath.CreateViewAccessor();
        }
    }
}