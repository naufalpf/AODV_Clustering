# MAODV - Node Clustering AODV

**Implementasi NS2 dari Paper "An Optimized AODV Protocol Based on Clustering for WSNs"**

     Naufal Pranasetyo Fodensi
     5115100057
     Kelas Jaringan Nirkabel 2018
---
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
  * Menghemat energi dan **memperpanjang network lifetime** dalam jaringan sensor nirkanel
  * Mencari rute paling **stabil** dari segi **sisa masa hidup node**
  
 #### 1.4 Penjelasan singkat solusi modifikasi AODV
 * Mengusulkan suatu algoritma clustering yang bernama AODV based on Node Degree Clustering and Second Search (AODVNDC-SS)
 * Di dalam algoritma ini node dikelompokkan sesuai dengan derajat nodenya untuk mengurangi propagasi pesan control menggunakan cluster heads.
 * Digunakan gateway umum atau gateway kooperatif untuk menghubungkan dua cluster heads (CHs) untuk mengurangi konsumsi energi cluster heads.


## 2. Modifikasi AODV
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
  * Berikut adalah proses dari Fase memilih gateway. 
  ![Flowchart](/img/flow2.jpg)
  

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

#### 2.3 Modifikasi yang dilakukan
1. Membuat global variable baru yang berfungsi untuk menyipan jumlah tetangga dari tiap node dalam *aodv_packet.h* pada hdr_aodv_reply berupa **rp_chid** dan menggunakan ++ nb_insert dan -- nb_delete.
2. Menambah field dan array **CH** dan **NodeNeighbor** untuk menyimpan ID node cluster head dan jumlah tetangga tiap node pada *recv request dan recv reply*
3. Menambah atribut koordinat dan **CH ID** neighbor node pada class AODV_Neighbor dalam file *aodv_rtable.h*
4. Melakukan request ke node tetangga dan simpan informasi dari node tersebut berupa berapa jumlah tetangganya, dan labeli node dengan jumlah tetangga terbanyak menjadi **Cluster Head** menggunakan logika "if else" pada recv reply pada *aodv.cc*
5. Cluster Head Melakukan re-broadcast **RE-RREQ** ke node lain untuk mendeklarasikan dirinya ialah Cluster Head pada recv request
6. Jika ada node penghubung antara dua Cluster Head, node tersebut menjadi gateway biasa. Jika tidak, maka menjadi **Cooperative Gateway** yang berguna untuk menjamin komunikasi antar node
7. Ketika node destinasi menerima pesan RE-RREQ. Node tujuan mengirim ulang pesan RREP (RE-RREP) ke node sumber dengan reverse route, ketika node sumber menerima pesan RE-RREP, rute lebih pendek akan dibuat.

#### Link Referensi
- https://ieeexplore.ieee.org/document/8343729
- https://arxiv.org/pdf/1007.4065.pdf
- http://intip.in/RA2018
- https://gitlab.com/naufalpf/maodv-nodeclustering
