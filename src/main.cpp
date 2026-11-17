#include <iostream>

#include "Archive/Packer.h"

int main()
{
    Archive::Packer packer;
    
    Error e = packer.Create("../Test", "archive");
    
    if (e)
    {
        e = packer.Open("../Test/archive.babar");
        if (e)
        {
            std::cout << e.Format() << "\n";
            return e.Exit();
        }
    }
    
    e = packer.Add("../Test/test.txt", "test", Archive::FileFlags::None, Archive::FileType::Text);
    std::cout << e.Format() << "\n";
    
    e = packer.Add("../Test/testVE2.txt", "test2", Archive::FileFlags::None, Archive::FileType::Text);
    std::cout << e.Format() << "\n";
    
    e = packer.Add("../Test/babar.jpg", "image", Archive::FileFlags::None, Archive::FileType::Binary);
    std::cout << e.Format() << "\n";
    
    e = packer.Remove("test2");
    std::cout << e.Format() << "\n";
    
    e = packer.Unpack("../Test/Unpack");
    std::cout << e.Format() << "\n";
    
    return 0;
}
