[Debug](https://www.mongodb.com/)
# MAODV - Node Clustering AODV

**Implementasi NS2 dari Paper "An Optimized AODV Protocol Based on Clustering for WSNs"**

     Naufal Pranasetyo Fodensi
     5115100057
     Kelas Jaringan Nirkabel 2018
---
## Outline
  - [1. Konsep](#1-konsep)
  - [2. Implementasi](#2-implementasi-modifikasi)
  - [3. Testing](#3-testing)
  - [4. Referensi](#4-referensi)
   <!--  - [1.1 Deskripsi Paper](#11-deskripsi-paper) - [1.2 Latar Belakang](#12-latar-belakang)- [1.3 Tujuan Clustering AODV](#13-tujuan-clustering-aodv)- [1.4 Solusi Modifikasi](#14-solusi-modifikasi-aodv) --><!--  - [2.1 Penjelasan Modifikasi](#21-penjelasan-singkat-modifikasi-aodv)- [2.2 Cara Kerja](#22-cara-kerja-clustering-aodv)- [2.3 Modifikasi](#23-modifikasi) -->
## 1. Konsep
#### 1.1 Deskripsi Paper

Implementasi MAODV didasarkan pada paper berikut :

* Judul : **An Optimized AODV Protocol Based on Clustering for WSNs**
* Penulis : Yan Feng, Baihai Zhang, Senchun Chai, Lingguo Cui, Qiao Li
* Sumber : https://ieeexplore.ieee.org/document/8343729

#### 1.2 Latar Belakang
* Latar Belakang dibuatnya Clustering AODV adalah:
     * Proses membangun rute baru di dalam protokol AODV dengan menggunakan mekanisme flooding membutuhkan banyak energi.
     * Jaringan sensor nirkabel memiliki beberapa keterbatasan. Salah satunya adalah keterbatasan dalam hal energi.
     * Ketidaksesuaian antara banyaknya energi yang dibutuhkan saat membangun rute baru di protokol AODV dengan keterbatasan energi yang dimiliki suatu jaringan sensor nirkabel.

#### 1.3 Tujuan Clustering AODV
* Tujuan dari Modifikasi adalah :
  * Mencari rute paling **optimal** dari node asal ke node tujuan pada jaringan MANET duntuk mengurangi **propagasi pesan kontrol**
  * Menghemat energi dan **memperpanjang network lifetime** dalam jaringan sensor nirkabel
  * Mencari rute paling **stabil** dari segi **sisa masa hidup node**

#### 1.4 Solusi Modifikasi AODV
 * Mengusulkan suatu algoritma clustering yang bernama AODV based on Node Degree Clustering and Second Search (AODVNDC-SS)
 * Di dalam algoritma ini node dikelompokkan sesuai dengan derajat nodenya untuk mengurangi propagasi pesan control menggunakan cluster heads.
 * Digunakan gateway umum atau gateway kooperatif untuk menghubungkan dua cluster heads (CHs) untuk mengurangi konsumsi energi cluster heads.


## 2. Implementasi Modifikasi
![CH](/img/ch.jpg)

#### 2.1 Penjelasan singkat modifikasi AODV
Paper berjudul **An Optimized AODV Protocol Based on Clustering in WSNs** ini mengusulkan suatu algoritma clustering yang bernama AODV based on Node Degree Clustering and Second Search (AODVNDC-SS) untuk mengontrol mekanisme flooding saat membangun suatu rute baru di protokol AODV. Di dalam algoritma ini node dikelompokkan sesuai dengan derajat nodenya untuk mengurangi propagasi pesan control. Node yang memiliki jumlah tetangga terbanyak akan menjadi **Cluster Head**. Pada saat yang sama, digunakan **gateway alternatif** dan **gateway kooperatif** untuk menghubungkan dua cluster heads (CHs) untuk mengurangi konsumsi energi cluster heads. Dengan menggunakan algoritma clustering ini akan menghemat energi dan memperpanjang network lifetime dalam jaringan sensor nirkabel.

#### 2.2 Cara Kerja Clustering AODV
![Flowchart](/img/flow.jpg)

* Cara kerja Clustering AODV :

  **1. Fase memilih Cluster Head**
     * Dalam fase ini setiap node mengirimkan hello message ke node lainnya untuk menentukan jumlah node tetangga. Sebuah node yang memiliki jumlah tetangga terbanyak akan menjadi cluster head.
     * Cluster heads mengirimkan pesan yang berisi ID nya ke seluruh node tetangganya. Dan ketika node tetangga menerima pesan yang berupa ID dari cluster head, node tetangga tersebut akan membalas dan bergabung dengan cluster. Berikut adalah proses dari fase memilih cluster head
     
  **2. Fase memilih Gateway**
    * Dua kategori gateway digunakan. kategori gateway yang pertama adalah gateway umum yang mendukung dua cluster head untuk berkomunikasi satu sama lain. 
    * Kategori gateway yang kedua adalah cooperative gateway yang harus bekerja sama untuk menjamin komunikasi antara dua cluster heads. Sebuah node bisa memilih salah satu dari dua kategori gateway untuk mendukung cluster heads yang berbeda.
  
  **3. Fase Pemilihan Rute**
    * Ketika sebuah node akan mengirim data ke node tujuan, node tersebut memeriksa apakah node tetangganya mengandung node tujuan. 
    * Jika iya, data akan dikirim ke node tetangga secara langsung. Jika tidak, node tersebut akan mencari tabel routingnya untuk mencari rute efektif ke node tujuan. 
    * Jika rute ditemukan, rute tersebut digunakan untuk mengirim data. Jika tidak, node tersebut akan mengirimkan pesan broadcast untuk menemukan jalur ke node tujuan. 

  
  **4. Fase Pengiriman Paket**
    * Jika node sumber adalah node biasa, node tersebut akan mengirim RREQ(Route Request) ke cluster heads dan cluster heads mengambil alih transmisi ini. 
    * Jika node sumber adalah cluster heads, ia akan mengirim RREQ ke common gateways atau cooperative gateway dan gateway mengambil alih transmisi ini. 
    * Jika node sumber adalah common gateway atau cooperative gateway, node tersebut akan mengirim RREQ ke cluster heads hilir atau cooperative gateway. ketika node tujuan menerima pesan RREQ, reverse route dibangun. Node tujuan mengirim RREP ke node sumber sesuai dengan reverse route. Ketika node sumber menerima pesan RREP, forward route akan dibangun.
    * Node sumber melakukan broadcast ulang RREQ (RE-RREQ) pesan. Berbeda dari pesan RREQ, RE-RREQ tidak di broadcast ke semua cluster heads dan gateway. RE-RREQ hanya akan dikirim ke node yang merupakan tetangga dari node pada rute sebelumnya. Ketika node tujuan menerima pesan RE-RREQ, reverse route baru akan dibuat. 
    * Node tujuan mengirim ulang pesan RREP (RE-RREP) ke node sumber sesuai dengan reverse route baru. Ketika node sumber menerima pesan RE-RREP, rute forward baru dibuat. Kemudian, data dapat dikirim melalui rute baru. Proses memilih rute yang lebih pendek dapat dilihat digambar dibawah ini
    
    ![CH2](/img/ch2.jpg)

#### 2.3 Modifikasi
1. Menambahkan field _neighbor_list_node, _count_neighbour, dan _CH_ID_ pada RREQ Message Format di **aodv.cc**
    - _count_neighbour_ merupakan variabel global untuk melakukan perhitungan jumlah tetangga tiap node
    - _neighbor_list_node_ untuk menyimpan berapa jumlah tetangga dalam tiap node
    - _CH_ID_ digunakan untuk menyimpan ID CH jika suatu node telah ditetapkan sebagai *Cluster Head* dari perhitungan setelah hello messages

2. Menambahkan field _rq_cluster_head_index_ pada fungsi **struct hdr_aodv_request** di file **aodv_packet.h** dan menambahkan header _CH_ID_ untuk melakukan penyimpanan node _cluster head_ dan fungsi _Modif_ serta_calculateCHID()_ untuk menginisiasi fungsi perhitungan pada file **aodv.h**

3. Melakukan inisialisasi paket yang dikirim pada fungsi **AODV::nb_insert** dan **AODV::nb_delete** yang ada di **aodv.cc**
    - **count_neighbour[index]** diberi nilai 0 yang nantinya akan mengalami perubahan setelah mengetahui jumlah tetangga
    - **nbhead.lh_first** 

4. Melakukan modifikasi melakukan perhitungan dan menentukan suatu node itu _Cluster Head_ atau bukan dengan menggunakan _max degree_  melalui fungsi **AODV::calculateCHID** dan **AODV::recvRequest** pada file **aodv.cc** dimana
    - _max degree_ merupakan threshold dimana node dengan tetangga paling tinggi akan dijadikan kandidat dan acuan pemilihan cluster head
    - jika _CH_ID_ = -1, maka node tersebut otomatis menjadi _gateway cooperative_ dan _gateway alternatif_

5. Modifikasi **AODV::sendRequest**, **AODV::sendHello**, dan **AODV::recvHello** yang digunakan oleh sebuah node untuk mendeklarasikan dirinya ialah _Cluster Head_ ke node lain melalui re-broadcast **RE-RREQ**
    - _CH_ID == -1_ Jika CH_ID belum ditemukan diawal, lakukan perhitungan. Jika sudah ditemukan, broadcast ke node lain
    - _nodes_count_ untuk mengetahui sudah berapa jumlah node yang sudah dihitung tetangganya
    -  _Node *sender_node_ dan _Node *receiver_node_ untuk mengetahui asal dan node tujuan

6. Jika ada node penghubung antara dua Cluster Head, node tersebut menjadi **gateway cooperative**. Jika tidak, maka menjadi **gateway alternatif** yang berguna untuk menjamin komunikasi antar node selain CH. Lalu membandingkan rutenya hanya melewati _Cluster Head_ saja. Jika tidak, maka paken akan di drop dan _routing table_ akan diupdate

## 3. Testing
#### Testing Skenario Paper
* Pada paper  **An Optimized AODV Protocol Based on Clustering for WSNs** menggunakan skenario 15 node statis.
* Digunakan shortest path untuk melakukan pengiriman paket serta reverse routenya. Berikut adalah hasil debugnya dan simulasi [Debug](/testing-skenario-paper) pada NS-2 dimana _node 0_ adalah source node dan _node 14_ adalah destination node:
![CH2](/img/start.jpg)
![CH2](/img/end.jpg)


### 4. Referensi
- https://ieeexplore.ieee.org/document/8343729
- https://arxiv.org/pdf/1007.4065.pdf
- http://intip.in/RA2018
- https://gitlab.com/naufalpf/maodv-nodeclustering
