/*
    Kiwi
    Random number generation (for Zobrist hashing)

    Copyright (c) 1999-2004,2005 Alessandro Scotti
    http://www.ascotti.org/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "random.h"

static const Uint64 Random64[] = {
    MK_U64(0x1a8326830d78d74d), MK_U64(0x5826ef0a2ba183b1), MK_U64(0xdda7c19c3511b556), MK_U64(0x5ec87bb0e4814c44), 
    MK_U64(0xf30140233d8e6539), MK_U64(0x302f25f8ac2f2c62), MK_U64(0xa230ce18ce5a16da), MK_U64(0x5884bf917c02e8c6), 
    MK_U64(0x1cff3ffd65a54411), MK_U64(0x18e2b63480a3819d), MK_U64(0x7b26187d236a1db8), MK_U64(0x52e21abb0f51aeb6), 
    MK_U64(0xffdb3d3bed379c15), MK_U64(0x086190c442fd5b02), MK_U64(0x8d41e67bd2f6e496), MK_U64(0x6083247ecbe46cf6), 
    MK_U64(0x3e026cc77717c292), MK_U64(0xc607aa410ff35e64), MK_U64(0x69bb2ac1178da023), MK_U64(0x23efbd35610a266e), 
    MK_U64(0xdbe719edd5ff74d5), MK_U64(0xf76fcbb8061e6fcd), MK_U64(0xbf96cf78a39a3cc7), MK_U64(0x346a92e6cc871386), 
    MK_U64(0x756dc7125188700f), MK_U64(0x33b0a9f4c74e14a0), MK_U64(0xfc742c1e94f07a75), MK_U64(0x696e18def74a47c9), 
    MK_U64(0x38bf9562bec22f56), MK_U64(0x5c355a5628223e56), MK_U64(0x8374f320a4d38760), MK_U64(0xca5e5fa18060d34b), 
    MK_U64(0x0a0b5039b1bdd718), MK_U64(0x55573d35b37ff4fb), MK_U64(0x89734233f20fb120), MK_U64(0x805668c21cb1fcca), 
    MK_U64(0x27ac0450bc5276d5), MK_U64(0xdf451ac04899acfe), MK_U64(0xa4a609b49ba3c777), MK_U64(0xf0e7372521cf923d), 
    MK_U64(0xa96dcf7cf132f213), MK_U64(0x736f48698910cacc), MK_U64(0xeb7dea54013b16b9), MK_U64(0x019e1855a3ee88c1), 
    MK_U64(0x606b69da49ed7553), MK_U64(0x0e902daaee851965), MK_U64(0x85f42ed173a42ce5), MK_U64(0x803b8107586d28b2), 
    MK_U64(0x43671ab332141936), MK_U64(0xcc5b192c8909bbb1), MK_U64(0x38f4ec66380eb5e9), MK_U64(0x7d39b2c6b2ae0e95), 
    MK_U64(0x53387d423b151954), MK_U64(0x47c099b395c78469), MK_U64(0x0a3e77c379105c0c), MK_U64(0xaf4d9017f6654873), 
    MK_U64(0xb9038aafd22bf159), MK_U64(0x585a271a060ea847), MK_U64(0x518edea029f3259b), MK_U64(0x3e73542094f751c8), 
    MK_U64(0x7f87d1a892d449dc), MK_U64(0x9bb8580f28cc6a93), MK_U64(0x132483f638fdfd54), MK_U64(0x0a2795a1a1750a41), 
    MK_U64(0x8d38f00e96c1fc8e), MK_U64(0x3d4f6f323180efe4), MK_U64(0x51d831fcc8ff7eb0), MK_U64(0x197cdda6e3e78c14), 
    MK_U64(0xc7a593704a7c7de6), MK_U64(0x9c5875e91158005b), MK_U64(0x8897e8dc06ca6449), MK_U64(0xb830d660344d8a91), 
    MK_U64(0x370a5ee2eabadbf8), MK_U64(0x32282e220b3cfe15), MK_U64(0xc23953f649ecec85), MK_U64(0xb33a8749853ab178), 
    MK_U64(0x4f8d7a58a6b7bcb2), MK_U64(0xffff9b9cd1044842), MK_U64(0xcddb4a831fe51c45), MK_U64(0x47467beced24a432), 
    MK_U64(0xfc1c3a0eeb3abd0b), MK_U64(0x73e359071d8d2245), MK_U64(0x2246f3535b38a6f3), MK_U64(0x3b8b2f65ec6b47f7), 
    MK_U64(0xd076d75851d9ba36), MK_U64(0x0f6bb4511213c7dd), MK_U64(0xc9a5ff93906a6515), MK_U64(0xecc726be84a3d665), 
    MK_U64(0x1f76acea47edfd4b), MK_U64(0x9c2f90ed7669691e), MK_U64(0xcf8ffc80808cde10), MK_U64(0x960ccf2dfabec810), 
    MK_U64(0xca6ab01114c4e54e), MK_U64(0x20252ba33125fcff), MK_U64(0xa8d4e18c8ccf3e97), MK_U64(0xa56f4906f150aa14), 
    MK_U64(0xea06ef3c02cf282c), MK_U64(0x61fbd64d26fc6763), MK_U64(0xca7cf343e11ebe22), MK_U64(0x4b575209634bf2af), 
    MK_U64(0xf372f49f4567d19c), MK_U64(0x67ddd7e52f4e3212), MK_U64(0xbff6fce3d885c465), MK_U64(0x9f115f2e59cb37c0), 
    MK_U64(0xdd051e68ec9900c6), MK_U64(0xb1589680f839b88e), MK_U64(0x695c413988e154bd), MK_U64(0x8b4ef8e79b9b6831), 
    MK_U64(0xd2a4b3d44256641c), MK_U64(0xd2ba675d48dd6e57), MK_U64(0x97623e6c93c52beb), MK_U64(0x9fd7c5189ffab5f5), 
    MK_U64(0x4aade8324ae3cca5), MK_U64(0xe74fddf0761387e1), MK_U64(0x3247460f5605f6a7), MK_U64(0xd90297bb7b656941), 
    MK_U64(0x4b2f369dad9e9d78), MK_U64(0x87fc7c978e74b24a), MK_U64(0x8be38c2cf5ad1193), MK_U64(0x7ec7d0c8742d69c1), 
    MK_U64(0x103d6e3837a5bc4f), MK_U64(0xee03a94558c4b721), MK_U64(0xde1dfe287b70df5e), MK_U64(0x5bad6439b65d0c60), 
    MK_U64(0x9ab769d790a84c2e), MK_U64(0xa84f53b6715c1226), MK_U64(0xe2400c3b00d11de8), MK_U64(0xda028ac73d45d328), 
    MK_U64(0xfb1535bb2f8fb2bb), MK_U64(0xdaedcae8db327de2), MK_U64(0xbf97968f6ff7d662), MK_U64(0xdeb68182fcedb8c2), 
    MK_U64(0xdcd26b736610718c), MK_U64(0x0496ba0eced54cd5), MK_U64(0x307454c487ed3de2), MK_U64(0xc236d8aa5204716a), 
    MK_U64(0xb3a6bf597d1c0a0e), MK_U64(0x69f2b9c0c67e60a4), MK_U64(0x58b7451eab99e1d5), MK_U64(0xb96c8e97f40567dd), 
    MK_U64(0xbb883abd6e7b456b), MK_U64(0xebddfe81c2f7e4cd), MK_U64(0x7f44215f70514d6f), MK_U64(0xf6489def3de9b43e), 
    MK_U64(0x9c0623e02ca127b8), MK_U64(0xa68b4cf0791aa9ac), MK_U64(0x3e17f6ebd152f952), MK_U64(0x5d72bd9ae56dfdad), 
    MK_U64(0xbb2f4894a3c8bf88), MK_U64(0x7dfff39455043d17), MK_U64(0x8043fcc98c5e511e), MK_U64(0xa52794bce0953e25), 
    MK_U64(0x53bda793693616b5), MK_U64(0xbec5640ec0039362), MK_U64(0x32d1110503f0908e), MK_U64(0x4db12a36a037e451), 
    MK_U64(0x1177bd4474d259db), MK_U64(0xfb963f11c6c8e148), MK_U64(0xdee0ef5687881264), MK_U64(0x8bef912b6a966a41), 
    MK_U64(0x974ed24b01c2fd54), MK_U64(0xf1ea6b4dca588d62), MK_U64(0x883de0715a2939be), MK_U64(0xff58e5594dc6afe8), 
    MK_U64(0xe574c381771dbf0d), MK_U64(0xdacaf271c6a19353), MK_U64(0x55c8846cadbef577), MK_U64(0xae862cc53a4fe60e), 
    MK_U64(0xa74734b0c39a6254), MK_U64(0x3382e1425ad0a089), MK_U64(0xd906b4cdee7ecfd3), MK_U64(0xe4258c2a6984dfdf), 
    MK_U64(0x7722d702cd623f3f), MK_U64(0xac81d2606df7dc07), MK_U64(0x9eb69c74a56b9534), MK_U64(0x76ab34a08a7b11a0), 
    MK_U64(0xf14367667aedfa39), MK_U64(0xdd25a13531e0e321), MK_U64(0xcf7991d45f27153d), MK_U64(0xdc40b437caaf62ca), 
    MK_U64(0x57552c3710e704c4), MK_U64(0x2fb917068da411b5), MK_U64(0xf56325c2c21c5e5e), MK_U64(0xb753b1eb8794fb05), 
    MK_U64(0x55603534f18c12cb), MK_U64(0xeb0b2bcf18d96cbb), MK_U64(0x6c88059f9bad0003), MK_U64(0xbfe8d8b0ea77b1ea), 
    MK_U64(0xad6900635e038115), MK_U64(0xa7ec5dda43b247a5), MK_U64(0x89823bac4dd84bee), MK_U64(0x012f18ac22d3c151), 
    MK_U64(0xd9599a46d707d4c6), MK_U64(0x80d5815ee5a6b7d5), MK_U64(0x4618693987d94bd8), MK_U64(0x0fba5c2f27eed63f), 
    MK_U64(0x634c440346807714), MK_U64(0x7ce9ad4783318559), MK_U64(0xc942aed780e0de1d), MK_U64(0x357c4d5083d59401), 
    MK_U64(0x1d0ac84b32a53b45), MK_U64(0x419b09dc4afc315b), MK_U64(0x533b0b417399febb), MK_U64(0x1943b2ffae5fb24a), 
    MK_U64(0x9935501be8ac18b3), MK_U64(0xf132c805ae078317), MK_U64(0x78123d109d7f2828), MK_U64(0xac8d9aedde995159), 
    MK_U64(0xd5c1e0a5de9f22c5), MK_U64(0x4bad1dacac6fb322), MK_U64(0x73e2a4a4495fe594), MK_U64(0xf922f2455c50a10b), 
    MK_U64(0x5e3c3eb9f436224b), MK_U64(0x8602fb21fa0403cd), MK_U64(0x22f0cd0d2aacade9), MK_U64(0x631fe3e025df1804), 
    MK_U64(0xf70c9507905c7a66), MK_U64(0xb26d15ad49dff4ab), MK_U64(0x4d2bee656c725b3c), MK_U64(0x68505cf8465fb10a), 
    MK_U64(0x5247bf9adc1c2382), MK_U64(0x52f1a5bec8ba05f5), MK_U64(0x2e3115197038278c), MK_U64(0xf0a09a6d4993dfe8), 
    MK_U64(0xad3e4bdbc368b367), MK_U64(0x2461f04f7583f6cd), MK_U64(0xa8ba7044330d3cd6), MK_U64(0x8dd6145552b78c36), 
    MK_U64(0x63457ce0a4e9aafd), MK_U64(0xfcb7f96ea7ec836a), MK_U64(0x70740ceeed25e157), MK_U64(0x9ea2b2c0fd1de2c2), 
    MK_U64(0x33b61737078a0ec5), MK_U64(0x976e304881059246), MK_U64(0x88b08a48c7c46179), MK_U64(0x4b97097eb361d896), 
    MK_U64(0xe239c7bd0217dd97), MK_U64(0x76d5ea118ae83130), MK_U64(0x5ef8a613b7b626c0), MK_U64(0xb9bc6e4beafc7c1c), 
    MK_U64(0x62895c4bc6a8553d), MK_U64(0x98c2d02d36d48bb9), MK_U64(0xe386c379cfcae9cf), MK_U64(0x5382fc88b24b935c), 
    MK_U64(0x500a4819c24142a3), MK_U64(0x20f64d7c092d2952), MK_U64(0x2c0ff7d49021acc0), MK_U64(0x7db6940e0ad2679c), 
    MK_U64(0x60ee6fcb5d172d6d), MK_U64(0x67ef290c0de9e2db), MK_U64(0xe8953922f207b0d2), MK_U64(0x80d5ba98804cc8f0), 
    MK_U64(0x0ebb9a2a5761b877), MK_U64(0x6bcc5af2e2544387), MK_U64(0x604c79aa0e231e66), MK_U64(0xa8351da40de5c749), 
    MK_U64(0xae5710142d89b72e), MK_U64(0xdb7ec05e578fc058), MK_U64(0x436999c51e2e699f), MK_U64(0x693f17a5ad2cc5c7), 
    MK_U64(0x274f0b40b83ac1a9), MK_U64(0xe6741a6ec5905375), MK_U64(0x351598198a0c1ced), MK_U64(0x29c30b6efeefa448), 
    MK_U64(0x71c69020a60d2b7f), MK_U64(0x9e87aff2af5ed27f), MK_U64(0x5c2db5febb2ec8e0), MK_U64(0x4c4187fd51d1f9b7), 
    MK_U64(0x76ea72865273772a), MK_U64(0x899902f417d7c749), MK_U64(0x4d8b0b5bf7069c92), MK_U64(0x713d0a4a150e415e), 
    MK_U64(0xd82ff15f13b3aa5b), MK_U64(0xaa908895ad2162e3), MK_U64(0xda8eecbd05272c78), MK_U64(0xdb21c5631e5462c9), 
    MK_U64(0x925914a41b5e3671), MK_U64(0x027544ebd5989dc8), MK_U64(0x5691fa13828f5212), MK_U64(0x94aa29a18f767e58), 
    MK_U64(0xd7219a7c3206769b), MK_U64(0xb53bba489d44f534), MK_U64(0x2c8ed9d30e7ed340), MK_U64(0xac9abe3f45b13ee5), 
    MK_U64(0x59539f74a864b378), MK_U64(0x428f877422486671), MK_U64(0xeac9ca7795a04d50), MK_U64(0xf8300e79b3b88e9c), 
    MK_U64(0xf42e881b617b692c), MK_U64(0x7e0da60c36e3d0e7), MK_U64(0x209d4a11e186311d), MK_U64(0xd3f12ad30e387473), 
    MK_U64(0xaf519b93e9ffde02), MK_U64(0xca99abef3204050e), MK_U64(0x8a73ac63d7d904fd), MK_U64(0x4c9f94f1156c37ac), 
    MK_U64(0xbab51503b0db0e4a), MK_U64(0xe5ef69a5cc3a1807), MK_U64(0x41c5452340a4b46c), MK_U64(0x973e777341163ebc), 
    MK_U64(0x88caedc1309024d1), MK_U64(0xbf16ce0c302318eb), MK_U64(0xb80ec70bb7ab343e), MK_U64(0xc822635b568af4d4), 
    MK_U64(0x0d190fd670551c05), MK_U64(0xd5b3eb1ae2525b11), MK_U64(0x331688ec5684a110), MK_U64(0x30f0f2faa49a4345), 
    MK_U64(0x37a92c9019caefd2), MK_U64(0x5e0e66c4848b3695), MK_U64(0xe8726e5956b7cd67), MK_U64(0x7f2eea6a6b5c08cb), 
    MK_U64(0xb1f92e97cd457689), MK_U64(0x9e6916803ba0b6b6), MK_U64(0x319cc0b1b33fc841), MK_U64(0xb9ff2c423e18f908), 
    MK_U64(0x0fa3019891dbf779), MK_U64(0x0c9ef5db6e4ef74b), MK_U64(0x21e130dc5246f767), MK_U64(0x5350e3994800f631), 
    MK_U64(0x8fe3d4db4df15e98), MK_U64(0x289f2720fc34a6a6), MK_U64(0x0c50e879a28842d0), MK_U64(0x4015d6674e987caf), 
    MK_U64(0xc5fc05572b488367), MK_U64(0xd6c7576fac594657), MK_U64(0xa9fc7256d6987043), MK_U64(0x119f37f2bb662a1f), 
    MK_U64(0x2976c03936d2fead), MK_U64(0x6cca6662d5b12d26), MK_U64(0x2e3f235433746049), MK_U64(0xcc1a169676761264), 
    MK_U64(0xaeed26b2df988ce4), MK_U64(0x4ca35c4e913d7ca5), MK_U64(0x962be693661ef2d4), MK_U64(0x199721101d98d580), 
    MK_U64(0x5479b30661e9a44c), MK_U64(0xe797536410beaed6), MK_U64(0x0b4ef732e8eb2625), MK_U64(0x5de2b4c75347c3a3), 
    MK_U64(0x8d855bfd36c775d8), MK_U64(0xd663e2422dc0c15a), MK_U64(0xd4fb441179962afc), MK_U64(0xbb410431a972cb0b), 
    MK_U64(0xcdab0a9e5bf37846), MK_U64(0x6fd2f306c14f4bad), MK_U64(0x085d90ca17f396e1), MK_U64(0x505db5b516109588), 
    MK_U64(0xe7cd2e26dd052af8), MK_U64(0xc7e1c5badea8ca19), MK_U64(0xe345d6ed30d31441), MK_U64(0x31762e4e52ac1998), 
    MK_U64(0x74b9131e4abf0d8f), MK_U64(0xd0cd6b831b689ebb), MK_U64(0xd0babbaedaa21aa5), MK_U64(0x6a8c6e2b2a4da241), 
    MK_U64(0xdafcf23bb5cfc286), MK_U64(0x14277b0491f6c53c), MK_U64(0x17dbfa13148a8478), MK_U64(0xf32abf9ec6a0ed99), 
    MK_U64(0xec7252a40b91438f), MK_U64(0xfb9c4a22a055eed4), MK_U64(0x32e7e8613843058c), MK_U64(0xec966de911fdb70c), 
    MK_U64(0x06702ea7ba13633c), MK_U64(0x647ad0876da7e2e7), MK_U64(0x30d7417f0978ca4f), MK_U64(0x0a18832c739cb821), 
    MK_U64(0x761b8da840aa63a0), MK_U64(0x270fe7bef1b4bae3), MK_U64(0x49877e5c856f7bba), MK_U64(0xbf917e64595808a7), 
    MK_U64(0x1eef15a91a62c086), MK_U64(0x91ffd0fdb77c50cf), MK_U64(0x77a5a9f4115a07e2), MK_U64(0x1a03086becf24227), 
    MK_U64(0x10396ec1d57cf854), MK_U64(0x0b130472bacb6b33), MK_U64(0x1ea8cecfc0e7f71a), MK_U64(0x16a2be10241adf77), 
    MK_U64(0x32839e5462dba535), MK_U64(0x796ee577fd350546), MK_U64(0x363250fb70856da4), MK_U64(0x4d6843fb6d5ae107), 
    MK_U64(0xec2183697bf374ce), MK_U64(0x03c91ac33aa916e3), MK_U64(0xb8a5b4f9fe276d65), MK_U64(0xe52bfdbe1337d7d9), 
    MK_U64(0xe34679520f492f5a), MK_U64(0x7317e7b459715435), MK_U64(0xddb679ce651b1820), MK_U64(0xb138a63d1cc5ebdd), 
    MK_U64(0xff0df7d5514f0d26), MK_U64(0x36db20098bcdc28a), MK_U64(0x5fd86dcf6a074361), MK_U64(0x3d26e18a08124a9a), 
    MK_U64(0x4068ad641d54db28), MK_U64(0x9ed5aded09b47d49), MK_U64(0xebbc1748d259eb40), MK_U64(0x344440c2921b06f6), 
    MK_U64(0x3e0c51de0e6a14cb), MK_U64(0xbf113d96310eb861), MK_U64(0xf480630a446ef0d2), MK_U64(0xe2c8b823bf136101), 
    MK_U64(0x472414407f5aa54a), MK_U64(0x2f419a90e2f256ee), MK_U64(0xb1c785da413e5c25), MK_U64(0x64996eef78d0eaa4), 
    MK_U64(0xd9c139f69508569f), MK_U64(0xee794c73d98b9337), MK_U64(0x0c68394ff598ec45), MK_U64(0xe42d71a7d34966f4), 
    MK_U64(0xa3a9d53ab92e2863), MK_U64(0x02b3c46e6e9fdfcf), MK_U64(0xbfab6e1c288e52d8), MK_U64(0x62d87bbd495819dc), 
    MK_U64(0x4a6ae41d2878a1e8), MK_U64(0xad6d38439db9cbaf), MK_U64(0x925458b39262b6aa), MK_U64(0xd5c874736128117e), 
    MK_U64(0x046539a7d49d7b22), MK_U64(0xcf48bed3a6c9d085), MK_U64(0x1ea0095c19efb9fe), MK_U64(0xca7f776515616984), 
    MK_U64(0xe0429d8b9f99f467), MK_U64(0xd7bc231d153aa81a), MK_U64(0x0205fb29d3962e19), MK_U64(0x020fa52479f3a027), 
    MK_U64(0xfdb6bc19ddc29e2e), MK_U64(0x2cbdec65c12398a5), MK_U64(0xc5d0e2907ec05c8d), MK_U64(0x1947d5d31991fa99), 
    MK_U64(0xdd7b74cb9845f4a7), MK_U64(0xec1177fd89e98ba5), MK_U64(0x63f3838d0deb4652), MK_U64(0x6d4512a30af7004e), 
    MK_U64(0x2541c91df6485c2c), MK_U64(0x816d8becf1be064b), MK_U64(0xe93f09cbc8895949), MK_U64(0xb263c2064c44f0c1), 
    MK_U64(0x67ece40666dfb269), MK_U64(0x7cc1797c48b2a681), MK_U64(0x5201471edc65e722), MK_U64(0x751700e9017b23ae), 
    MK_U64(0x19bd3fdaf8dd7bbe), MK_U64(0x28367588d6a8a8f6), MK_U64(0xc3ecac7a01c7675a), MK_U64(0xa691a9fdd8ecbd23), 
    MK_U64(0x8415ff372d700452), MK_U64(0xa1b65d583207ea5b), MK_U64(0x4787627c82c7ceeb), MK_U64(0xd9f24ee34aabdb84), 
    MK_U64(0xf739520216271878), MK_U64(0x6f1175e5391e4d34), MK_U64(0x36de0682dd932644), MK_U64(0x8f79f222d70a393e), 
    MK_U64(0x7e591199d6dffcc2), MK_U64(0x032141bfab6e0419), MK_U64(0xae0b3cdaa8a07569), MK_U64(0xd42e0d6195b37ab5), 
    MK_U64(0x88f5e13a3e25dfcc), MK_U64(0xa77b9111b0645e37), MK_U64(0x42bd4d257bedead6), MK_U64(0xa50b9cf0ea9b0503), 
    MK_U64(0x25c1fc6f94dc2d96), MK_U64(0x3ec3e3fa6c42c69d), MK_U64(0xf511f5e7d038e31e), MK_U64(0xc9c351e8f4b3e736), 
    MK_U64(0xa3fce851698ab2e2), MK_U64(0xf84215241874f758), MK_U64(0x26dadff28a0132b4), MK_U64(0x6e70e84381b1ff7b), 
    MK_U64(0x752736275cff4541), MK_U64(0xac503d25aee2f123), MK_U64(0x5ddb98a78f221656), MK_U64(0x6e2a39abd05f8fa2), 
    MK_U64(0x3c9930335ad074c0), MK_U64(0xdf3c8514bb215c44), MK_U64(0x12b274f76a464240), MK_U64(0x4fe14df8c595fd1d), 
    MK_U64(0x9722260d3bc21ed0), MK_U64(0xdba34ce62a711801), MK_U64(0xf25cd0fcbec1d0e3), MK_U64(0x17d2e11a22ad6cfd), 
    MK_U64(0x0a7ea876cfd6ce19), MK_U64(0xf539feb7aeee76e4), MK_U64(0xafc6f3418125c010), MK_U64(0x4bfd66eed212296c), 
    MK_U64(0x75cef84a2b191d94), MK_U64(0x1c2595daba43a8b3), MK_U64(0x5d5e9c780b92040b), MK_U64(0x17ac3c524ebce5d0), 
    MK_U64(0x527ab4a3338f8ab4), MK_U64(0xce22026ac18084b4), MK_U64(0x098023348d449ff2), MK_U64(0x2388136e9d3cad95), 
    MK_U64(0x389a37da9e8398dd), MK_U64(0x2c321c45f01b07c2), MK_U64(0xb48c07a1c5776d7c), MK_U64(0x60e0edd41c405e63), 
    MK_U64(0x0535dc2dafdfbc3c), MK_U64(0xb048b54875da6695), MK_U64(0x4dbc4c0a06e991cd), MK_U64(0x58b2255710443062), 
    MK_U64(0x62c854f69464801b), MK_U64(0x84fe797cdd760a1b), MK_U64(0xd87ec534987aceaa), MK_U64(0x0ad9192c37c9ff47), 
    MK_U64(0x627b4a1fde1bc088), MK_U64(0x2dfbf519b392944b), MK_U64(0xb0e03599abffbcd8), MK_U64(0xcda87f8bd29fa2ea), 
    MK_U64(0x2f31b279dfa97491), MK_U64(0x4cbe02203c3be93f), MK_U64(0xa3c3554042d6176d), MK_U64(0x7fcf27881e9ec6b2), 
    MK_U64(0x5b8d51d5f0386b1a), MK_U64(0xe57753d41b12a8a2), MK_U64(0x3113ed0d44767e9d), MK_U64(0x339cfc26de7cd0f2), 
    MK_U64(0x9fb714066b1ca7ce), MK_U64(0x398bbd304820ce0e), MK_U64(0xbcf7118c47fc6386), MK_U64(0x0c1cc76b712e0485), 
    MK_U64(0x02066c4ca4c1c8e5), MK_U64(0x5d40b1b4862bdbe3), MK_U64(0xac1cf9563b793ed1), MK_U64(0x7908fcff4704fe84), 
    MK_U64(0x1ce2a6bf45575d9b), MK_U64(0xffa4cffbcba33128), MK_U64(0x18d09cbf55121184), MK_U64(0x6a0bac65236b279c), 
    MK_U64(0x4d43bc068afb66b8), MK_U64(0xf64af5f48df3038f), MK_U64(0xd2510402546db875), MK_U64(0x9cf99fc6a9fe2a87), 
    MK_U64(0x7e8ddebe61c1d8c5), MK_U64(0x558822252d07214d), MK_U64(0xf2ce02d3d84e74e1), MK_U64(0x037bd3f729cbad12), 
    MK_U64(0x54013fceaae3a4c1), MK_U64(0x5d956807a595c202), MK_U64(0x60e0476c2850d8eb), MK_U64(0x4229083d579f0fd2), 
    MK_U64(0x023229a9bc6048b6), MK_U64(0xc41919072233f936), MK_U64(0x05237d4240cf6a76), MK_U64(0xf347c54ab79551b1), 
    MK_U64(0x116e17d71a7014bf), MK_U64(0x492c0c7c5bb1c570), MK_U64(0x2b57db23935ef3ee), MK_U64(0x5a4831d8feff5351), 
    MK_U64(0x2aded4d2917a1141), MK_U64(0x78078f7f85c48a2b), MK_U64(0xfec2504a96bb0505), MK_U64(0x867516a6f42ae1c1), 
    MK_U64(0x00697075146b0f01), MK_U64(0x2d9dc5a15bb3de47), MK_U64(0xd58b6cbbdef87c5d), MK_U64(0xfe9af40915e28ab2), 
    MK_U64(0xe53bbbb16d1a739c), MK_U64(0x617671697a30d691), MK_U64(0x27a74e96ad4cad43), MK_U64(0x65558e31d383bb05), 
    MK_U64(0x32d12ba145d0195b), MK_U64(0x16ffc9958990c229), MK_U64(0x77a3c81e0f91f63a), MK_U64(0x7d0b2976d21bcf85), 
    MK_U64(0xf74840b5b7a706a1), MK_U64(0x5992af84ca10d086), MK_U64(0x57167f979539887f), MK_U64(0xe537ff2c309004bc), 
    MK_U64(0x676451ac48e29e7f), MK_U64(0x823fe06c9b2e6b7d), MK_U64(0x17380039461f914c), MK_U64(0x52dc261d248c6fe2), 
    MK_U64(0xaf3c4081a0a032d2), MK_U64(0x48ee9f9982f60639), MK_U64(0x3b5ab1bd9e66ec1f), MK_U64(0xc1b599d266c4c91e), 
    MK_U64(0xad49eeb9c2f7c908), MK_U64(0xb4dba1e13af0bdad), MK_U64(0xfa0a46af005ab01b), MK_U64(0x6c6cbddeca9c783c), 
    MK_U64(0xe9b638e4f58cf50b), MK_U64(0x3edad50a39b980fa), MK_U64(0x4b92988a2f962a14), MK_U64(0x7897d553a2c275b7), 
    MK_U64(0x51cca8863db736ee), MK_U64(0x931ed7de63e8c3f2), MK_U64(0x1b5708eb7465d3cb), MK_U64(0x87a9a393498c5f61), 
    MK_U64(0xd97d6778e1de3845), MK_U64(0xf3cb62a4707e1e27), MK_U64(0x32301daa4171a98a), MK_U64(0xb03656ab9f3dc34c), 
    MK_U64(0xe581c4ec1e839288), MK_U64(0x1eea754ec4020da6), MK_U64(0xd0722a1347388c88), MK_U64(0x75e844e7cf8e18dd), 
    MK_U64(0xa1bcd5a3ebf2a58b), MK_U64(0x9d45f360158401f9), MK_U64(0x7877bfd98f2ff6ee), MK_U64(0x35514ea436297ffb), 
    MK_U64(0xab66ba9d7c6d0c18), MK_U64(0xffc4d6d49795cd77), MK_U64(0x37f335090a135f36), MK_U64(0xd839d26f5572c0f5), 
    MK_U64(0xccf69e099f2a37b4), MK_U64(0x08f21b1fc81da3c2), MK_U64(0x423f6160f3347d44), MK_U64(0xd03fa97f58c86792), 
    MK_U64(0x42a61e43b55ad28d), MK_U64(0x89fda043bde13a1e), MK_U64(0xf346db1ebe073c4e), MK_U64(0x54b71861273764e7), 
    MK_U64(0xeece6592245e6f03), MK_U64(0xcec56908dde3fd8a), MK_U64(0x4f21b93c83dcdd5a), MK_U64(0xb134afc67d84dad6), 
    MK_U64(0x744615f741723131), MK_U64(0xc569aa34f3129431), MK_U64(0x093d37b254c8ac24), MK_U64(0x07b253506b0700c2), 
    MK_U64(0x06977e4714643735), MK_U64(0xf0aa97c30d457031), MK_U64(0xd96264a53cacdb6f), MK_U64(0x8fd60c4ab5b41c57), 
    MK_U64(0xc2a303397b636aad), MK_U64(0x7dbf02fed7a7470a), MK_U64(0x77fa97338f3b1cbe), MK_U64(0xebdf8020012ed672), 
    MK_U64(0x6a90c09805de64ad), MK_U64(0xdd2b0a93c4b24c9a), MK_U64(0x93d32bda29b3fbef), MK_U64(0x7b34209bc25f7fc8), 
    MK_U64(0xefd3e27024be95c2), MK_U64(0xb8ba313c3a9b3cd3), MK_U64(0xc3de163cc40d0ef7), MK_U64(0x170dbdbe7366b4b6), 
    MK_U64(0x03c2f34dc68bd2ab), MK_U64(0xf7e6d9a7790facb1), MK_U64(0x24e1274a84c5d8b8), MK_U64(0x30141415869342ac), 
    MK_U64(0x8d11ac69da52cccb), MK_U64(0x07ef0e729aad73b8), MK_U64(0x6ae464e7f303c086), MK_U64(0x695eabd5be4387d9), 
    MK_U64(0x9685fa3ee0855b95), MK_U64(0x25c12f0744e83517), MK_U64(0xc926195d92bf42e5), MK_U64(0x03a8b17037e20471), 
    MK_U64(0x7f3d3f64d056dc17), MK_U64(0xd27415bbf0fb581e), MK_U64(0xad6e15c5ba74aab8), MK_U64(0x9426dd3284e6ab65), 
    MK_U64(0x9f26b810bcf1b256), MK_U64(0xe756aba3f49c1cfa), MK_U64(0x2b987cde6904c1e9), MK_U64(0x61ccfd210746db37), 
    MK_U64(0x35764737ca4034b7), MK_U64(0x65d03651f6af3530), MK_U64(0xbd72f19d46247469), MK_U64(0x0cb74823f0353ac9), 
    MK_U64(0xe6da1296dc2296fb), MK_U64(0x8837512dd20fbc32), MK_U64(0xe4ba5db5096c7d62), MK_U64(0x05dc14c336e5ebd4), 
    MK_U64(0xc29d79b372f32816), MK_U64(0x4ddc378caca2c1f0), MK_U64(0x458c118ea0cf4d8c), MK_U64(0xe223488871828b4c), 
    MK_U64(0xe62573df1f06b7a6), MK_U64(0x01542bdd35d1c5c7), MK_U64(0x88266e4c43df8585), MK_U64(0xab72faeee9f3059f), 
    MK_U64(0xa0174692c2b7bb77), MK_U64(0x1d74d78cdade7712), MK_U64(0xcebc0addcb62dbe5), MK_U64(0x4530d81b9ee9a99c), 
    MK_U64(0xb55d8382205b75a7), MK_U64(0xe2c5eb3e901441d1), MK_U64(0x5d1de57d690bc2eb), MK_U64(0x9e87e25e5afc8a7a), 
    MK_U64(0x0f83f5a5fd624fc9), MK_U64(0x395c41f1677b9089), MK_U64(0x962d16e97d7c028f), MK_U64(0x95ed77bf2c9d1b2b), 
    MK_U64(0xe68d3448f99ae753), MK_U64(0xa542d2765adf6dc4), MK_U64(0xb1b9293a7a20cbd1), MK_U64(0x41858b94a13db57d), 
    MK_U64(0x018c4287c8ce0136), MK_U64(0x06dd58e50ae43fea), MK_U64(0xcd7e9fc6627fa868), MK_U64(0xd67cb8288a23ba70), 
    MK_U64(0x4d06c71845fefe58), MK_U64(0x206e3d0b4fa5ec29), MK_U64(0x6fc43d8315af6d25), MK_U64(0xb4060853346b0bf8), 
    MK_U64(0x68a7427e99ea8a4f), MK_U64(0xce1feddf9cfeb441), MK_U64(0xfda2d00d61decf90), MK_U64(0xed2a840f00d6d26c), 
    MK_U64(0x31e1b9e1e96ba1ed), MK_U64(0xfe004ca0d51bd0e0), MK_U64(0x9b09dc3eac534570), MK_U64(0x7255fdb594162d3c), 
    MK_U64(0x9499b2a7cdf85d10), MK_U64(0x1c23a08c2e44683c), MK_U64(0x60412c7cdbf10a53), MK_U64(0xee4fd72ace3fce78), 
    MK_U64(0x4bc1cdbfc971d944), MK_U64(0x83579902f6b4d9e0), MK_U64(0x0cbf6d456a1d71bc), MK_U64(0xdc6b06a9eab0464a), 
    MK_U64(0xb990178b04d12bba), MK_U64(0x64bcbe0b9f19c3f8), MK_U64(0x9282016a38073215), MK_U64(0x9c111067375e649b), 
    MK_U64(0x03346d8cff65f832), MK_U64(0x0e00f46b55eb30f2), MK_U64(0x59a2c44f346a72c4), MK_U64(0x0b716721b46b9393), 
    MK_U64(0x26025bf1170a6a74), MK_U64(0x68ef6ce82bbf07f1), MK_U64(0xcff09b9412b066f0), MK_U64(0xb1a8da20cff4a63f), 
    MK_U64(0x5a975e513b08a582), MK_U64(0x9e6e89b614bd4b14), MK_U64(0x0def379a2d5292e9), MK_U64(0xd116f09f911d2b54), 
    MK_U64(0x81f5a4efaf58a627), MK_U64(0xd69d2a005699ae4a), MK_U64(0x857fa916a20d767b), MK_U64(0x54372f7f4735e117), 
    MK_U64(0xf7d651bfd96327a5), MK_U64(0xd043ea615b3acaa5), MK_U64(0xe8149e86b768eef0), MK_U64(0x859c2ac2d09ed6e7), 
    MK_U64(0xc33f065b9adcc9cb), MK_U64(0xcce6e83513a2eacf), MK_U64(0x8fb925a207ba34f4), MK_U64(0xa68a1d99208e41a1), 
    MK_U64(0xcc10e8f5ba369edd), MK_U64(0x020b367ecd7554d1), MK_U64(0xeaebbc0cdfb5ecd9), MK_U64(0x03d743b7f98f1d32), 
    MK_U64(0x809035ea48ae5cf8), MK_U64(0x9b075409c7a9434e), MK_U64(0x02f8daceded550df), MK_U64(0xcc1dbe8c20bc0049), 
    MK_U64(0x5bdc13ce43a185bb), MK_U64(0xbde96bb86d6d4a5a), MK_U64(0x863e6cd821d7a9ba), MK_U64(0xf217fd464320f707), 
    MK_U64(0x555fc99ad0d714a4), MK_U64(0xd7bb14fbc3133c6f), MK_U64(0xb2f764e3909d39ca), MK_U64(0x8e5fe8eccf80b1f1), 
    MK_U64(0x30ca2057373f9a86), MK_U64(0x2ce9b93e42adeb44), MK_U64(0x465e982d7c582ffe), MK_U64(0xa72447f07e6094cf), 
    MK_U64(0xb8147d4f727b4532), MK_U64(0x4ada8989029ef527), MK_U64(0x4368ce478da92417), MK_U64(0xa6f186e3f23aad1d), 
    MK_U64(0x07c2eb1ef66d2bc4), MK_U64(0xd89ecad4070b501b), MK_U64(0x97f962951cb2126a), MK_U64(0x6f9a69eab59c2ce5), 
    MK_U64(0x8cb95aa57d61afe2), MK_U64(0xceda0476ac238a5b), MK_U64(0x371b8a7610839d5d), MK_U64(0x4308e2e839313f6e), 
    MK_U64(0xa80d2a88d60347eb), MK_U64(0xd6e22df3877c1848), MK_U64(0x6ce5425b31afd796), MK_U64(0x63d74fb4d25cdab9), 
    MK_U64(0x0691a57e86162a52), MK_U64(0x55a8f2ced7a78742), MK_U64(0x22e0b30ba17fa1d5), MK_U64(0x543f3fd4d284179a), 
    MK_U64(0x01f399a75498d51a), MK_U64(0xdcde4a8ab8c5ba4d), MK_U64(0x3a5c48ddfa5ade78), MK_U64(0x4b8a5ebe61781c21), 
    MK_U64(0xe85bcf749c07c6d1), MK_U64(0xac205bd0b402293a), MK_U64(0x8f4b8e10031355a3), MK_U64(0x356f8550dc16440d), 
    MK_U64(0xe6f4ff158188c09f), MK_U64(0x8313dc609ec8e358), MK_U64(0x13b9e6c7f875f8c7), MK_U64(0x7cde83bfe564becc), 
    MK_U64(0xf4a5fc800af66bcf), MK_U64(0x22e4a05eb8a0fc14), MK_U64(0xbca6715e92fda15f), MK_U64(0x206fb4f6f0379bc7), 
    MK_U64(0x229a721c4688fd27), MK_U64(0x3ad52a5831d71c5a), MK_U64(0x4811eb21d90ab2cb), MK_U64(0x25dd93b91d96a860), 
    MK_U64(0xc1280c9b8099914a), MK_U64(0x8a8cf80c9fbb431a), MK_U64(0xb1dc3bc36f2a5b94), MK_U64(0xd07d77553ba87aaa), 
    MK_U64(0xec1778d1c25adc3a), MK_U64(0x872069039885a938), MK_U64(0x618272db2391526e), MK_U64(0x19bf76130a61ed34), 
    MK_U64(0xadc1662cadbef240), MK_U64(0xccd754653c3279ae), MK_U64(0xcd1c7823e8456836), MK_U64(0x186181f21864b0c1), 
    MK_U64(0x06f0e85c8cfb3f17), MK_U64(0x89ca61e638331a26), MK_U64(0xe88a66440449386d), MK_U64(0x13542f4cd42ea099), 
    MK_U64(0xabf2a8a1eaf08314), MK_U64(0x68d02715adeca19a), MK_U64(0x1c95f7d19636fb87), MK_U64(0x399544006e100327), 
    MK_U64(0x0d3734f018a5479f), MK_U64(0x3d7ccc7ee0adb207), MK_U64(0x3022bab1ef1f3716), MK_U64(0x0267f9a2d3a8a2f3), 
    MK_U64(0x7d9087f73d7be709), MK_U64(0xca6ac14b685f7ed9), MK_U64(0x4ae0683cb10ca969), MK_U64(0xece167717067f6b2), 
    MK_U64(0x671fdf702de120e0), MK_U64(0x607ceb75c7d2bcd4), MK_U64(0xb5899e1e244a4aaf), MK_U64(0x53dfc50c49e1d803), 
    MK_U64(0x8b9763f732c9dc1c), MK_U64(0x42595a276e879698), MK_U64(0xddb9f5e256cc30b0), MK_U64(0xe2f279434afd7442), 
    MK_U64(0x498dca89fb7db17f), MK_U64(0x3c10317d181d9934), MK_U64(0x19b39e60fbb00737), MK_U64(0xb662c2aa582d3e76), 
    MK_U64(0x273880c21c8d81ec), MK_U64(0xf016e0e8bac32ed5), MK_U64(0xe9754ce16e4d0e5f), MK_U64(0x9cc7fe94d4a34aa0), 
    MK_U64(0x690b2eca53f5c678), MK_U64(0xbbac60a4f4534c4f), MK_U64(0xef5c406f7ff2c339), MK_U64(0xc9d6022faa018f21), 
    MK_U64(0xf17de5d050f7fc0b), MK_U64(0x8e0d22c07275881a), MK_U64(0x81284c72978cc424), MK_U64(0xb1e57adfe9246e4a), 
    MK_U64(0x2c52fcf0708ed2ae), MK_U64(0xd079ca0ff40e127a), MK_U64(0x9f5e88545bc76aaf), MK_U64(0x59407990a3caf789), 
    MK_U64(0x9be56842638ad315), MK_U64(0xabe555bde12dde58), MK_U64(0xe3bbd40747cd860e), MK_U64(0x47df911add6f1264), 
    MK_U64(0x6b8440543dc4db62), MK_U64(0xc9497547e57d51f5), MK_U64(0x64e9983f8b69cfdb), MK_U64(0x20dd2349549db8e0), 
    MK_U64(0xd93eaf8ef9382df2), MK_U64(0x60e4e02b92a3a7f9), MK_U64(0x7db8b973135b43f3), MK_U64(0xe3ad795a46ba1159), 
    MK_U64(0x3ca1bee874f4d1a2), MK_U64(0x7a720771fa0af677), MK_U64(0x42be282a5e0ab871), MK_U64(0xd030327e0b6ecc9a), 
    MK_U64(0x63274f713d06020e), MK_U64(0x526be6d1c66a27fc), MK_U64(0x462dc09d137ab2a5), MK_U64(0x64112fbf964d6a3d), 
    MK_U64(0x40f15598184f362a), MK_U64(0x20f3194a7cd15f5f), MK_U64(0x0b73cdf52b846849), MK_U64(0xfe186e33eec6c590), 
    MK_U64(0x1582ca798fad4059), MK_U64(0xeb9a0491ad54a9ee), MK_U64(0x67cf343159331e75), MK_U64(0xec66396ce45c3c69), 
    MK_U64(0x21c0e1ee192a3c35), MK_U64(0xb7bab527a76e166c), MK_U64(0xcfa7814f1406a28e), MK_U64(0x3468b24706657616), 
    MK_U64(0x029fc687f63b3334), MK_U64(0x21b34679db44b003), MK_U64(0x09bb1b68a7064aa1), MK_U64(0x4042a4fbfa61d579), 
    0
};

// Random numbers extracted from the S-Box of Blowfish
static unsigned const X[55] = {
    0xd1310ba6, 0x98dfb5ac, 0x2ffd72db, 0xd01adfb7,
    0xb8e1afed, 0x6a267e96, 0xba7c9045, 0xf12c7f99,
    0x24a19947, 0xb3916cf7, 0x0801f2e2, 0x858efc16,
    0x636920d8, 0x71574e69, 0xa458fea3, 0xf4933d7e,
    0x0d95748f, 0x728eb658, 0x718bcd58, 0x82154aee,
    0x7b54a41d, 0xc25a59b5, 0x9c30d539, 0x2af26013,
    0xc5d1b023, 0x286085f0, 0xca417918, 0xb8db38ef,
    0x8e79dcb0, 0x603a180e, 0x6c9e0e8b, 0xb01e8a3e,
    0xd71577c1, 0xbd314b27, 0x78af2fda, 0x55605c60,
    0xe65525f3, 0xaa55ab94, 0x57489862, 0x63e81440,
    0x55ca396a, 0x2aab10b6, 0xb4cc5c34, 0x1141e8ce,
    0xa15486af, 0x7c72e993, 0xb3ee1411, 0x636fbc2a,
    0x2ba9c55d, 0x741831f6, 0xce5c3e16, 0x9b87931e,
    0xafd6ba33, 0x6c24cf5c
};

static int random_index = 0;

/*
    32-bit random number generator based on lagged Fibonacci sequence
    (see D. Knuth, "The Art Of Computer Programming", vol. 2)

    The original formulation is:

    X[n] = (X[n-55] + X[n-24]) mod m

    To implement the algorithm we make use of a precomputed table that gives 
    the values of X[0]...X[54] so that we have something to start with.
    If we set j=24 and k=55 then:
        X[n] = X[n-k] + X[n-j].
    Of course we don't want to keep the whole sequence in memory and luckily
    we don't have to, because when X[55] has been generated we no longer need
    X[0]! In fact, we can just override X[0] with X[55], then X[1] with X[56]
    and so on, thus reusing always the same locations.
*/
Random::Random()
{
    for( int i=0; i<55; i++ ) x[i] = X[i];
    j = 55 - 24;
    k = 55 - 55;
}

unsigned Random::get()
{
    int result = x[j] + x[k];

    x[k] = result;

    if( ++j >= 55 ) j = 0;
    if( ++k >= 55 ) k = 0;

    return result;
}

BitBoard Random::getBitBoard()
{
    Uint32 hi, lo;
    bool good = true;
    BitBoard result;

    // Test: 8/8/8/1p5r/p1p1k1pN/P2pBpP1/1P1K1P2/8 b - - bm Rxh4 b4; id "WAC.229";
    // ...huge difference in node counts when using standard or "true" random numbers!

    do {
        hi = get();
        lo = get();
        result = BitBoard(lo,hi);

        good = bitCount(hi) >= 8 && bitCount(lo) >= 8 && bitCount(result) >= 18 && bitCount(result) < 46;
    } while( ! good );

    if( Random64[ random_index ] != 0 ) {
        result = Random64[ random_index ];
        random_index++;
    }

    return result;
}
